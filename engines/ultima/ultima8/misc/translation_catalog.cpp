/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/array.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "ultima/ultima.h"
#include "ultima/ultima8/misc/translation_catalog.h"

namespace Ultima {
namespace Ultima8 {

namespace {

const uint32 MO_MAGIC = 0x950412de;
const uint32 MO_MAGIC_SWAPPED = 0xde120495;
const uint32 MO_MAX_SIZE = 64 * 1024 * 1024;
const char CONTEXT_SEPARATOR = '\x04';

bool parseHex(const Common::String &s, uint16 &value) {
	if (s.empty() || s.size() > 4)
		return false;
	uint v = 0;
	for (uint i = 0; i < s.size(); i++) {
		char c = s[i];
		v <<= 4;
		if (c >= '0' && c <= '9')
			v |= c - '0';
		else if (c >= 'a' && c <= 'f')
			v |= c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			v |= c - 'A' + 10;
		else
			return false;
	}
	value = v;
	return true;
}

} // End of anonymous namespace

TranslationCatalog::TranslationCatalog() : _skipped(0) {
}

void TranslationCatalog::clear() {
	_entries.clear();
	_barkContexts.clear();
	_skipped = 0;
	_language.clear();
}

Common::String TranslationCatalog::barkContext(uint16 classId, uint16 ip) {
	return Common::String::format("bark %04X:%04X", classId, ip);
}

Common::String TranslationCatalog::askContext(uint16 classId) {
	return Common::String::format("ask %04X", classId);
}

bool TranslationCatalog::canonicalContext(const Common::String &context, Common::String &canonical) {
	uint16 classId = 0, ip = 0;
	if (context.hasPrefix("bark ")) {
		Common::String id = context.substr(5);
		size_t colon = id.findFirstOf(':');
		if (colon == Common::String::npos ||
		        !parseHex(id.substr(0, colon), classId) ||
		        !parseHex(id.substr(colon + 1), ip))
			return false;
		canonical = barkContext(classId, ip);
		return true;
	}
	if (context.hasPrefix("ask ")) {
		if (!parseHex(context.substr(4), classId))
			return false;
		canonical = askContext(classId);
		return true;
	}
	return false;
}

// Same rules as the UTF-8 font decoding: anything it would draw with the
// legacy 8-bit fallback is rejected here.
bool TranslationCatalog::isValidUTF8(const Common::String &text) {
	uint i = 0;
	while (i < text.size()) {
		uint8 c = text[i];
		uint n;
		if (c < 0x80)
			n = 1;
		else if (c >= 0xC2 && c <= 0xDF)
			n = 2;
		else if (c >= 0xE0 && c <= 0xEF)
			n = 3;
		else if (c >= 0xF0 && c <= 0xF4)
			n = 4;
		else
			return false;
		if (n > text.size() - i)
			return false;
		for (uint k = 1; k < n; k++) {
			if ((static_cast<uint8>(text[i + k]) & 0xC0) != 0x80)
				return false;
		}
		i += n;
	}
	return true;
}

void TranslationCatalog::parseHeader(const Common::String &header) {
	size_t start = 0;
	while (start < header.size()) {
		size_t end = header.findFirstOf('\n', start);
		if (end == Common::String::npos)
			end = header.size();
		Common::String line = header.substr(start, end - start);
		if (line.hasPrefixIgnoreCase("Language:")) {
			_language = line.substr(9);
			_language.trim();
		}
		start = end + 1;
	}
}

void TranslationCatalog::addEntry(const Common::String &key, const Common::String &translation,
                                  const Common::String &name) {
	size_t sep = key.findFirstOf(CONTEXT_SEPARATOR);
	if (sep == Common::String::npos) {
		warning("[U8-L10N] %s: entry without context skipped: \"%s\"", name.c_str(), key.c_str());
		_skipped++;
		return;
	}

	Common::String context;
	if (!canonicalContext(key.substr(0, sep), context)) {
		warning("[U8-L10N] %s: unknown context skipped: \"%s\"", name.c_str(),
		        key.substr(0, sep).c_str());
		_skipped++;
		return;
	}

	Common::String source = key.substr(sep + 1);
	if (source.empty() || translation.empty() ||
	        source.contains('\0') || translation.contains('\0')) {
		// empty text, or a plural entry (forms are NUL separated)
		warning("[U8-L10N] %s: %s: empty or plural entry skipped", name.c_str(), context.c_str());
		_skipped++;
		return;
	}

	if (!isValidUTF8(translation)) {
		warning("[U8-L10N] %s: %s: translation is not valid UTF-8, skipped", name.c_str(),
		        context.c_str());
		_skipped++;
		return;
	}

	Common::String fullKey = context;
	fullKey += CONTEXT_SEPARATOR;
	fullKey += source;
	if (_entries.contains(fullKey)) {
		warning("[U8-L10N] %s: %s: duplicate entry skipped: \"%s\"", name.c_str(),
		        context.c_str(), source.c_str());
		_skipped++;
		return;
	}

	_entries[fullKey] = translation;
	// An answer is identified by its text, so a different text is simply
	// another answer; only a bark call site can show unexpected text.
	if (context.hasPrefix("bark "))
		_barkContexts[context] = true;
}

bool TranslationCatalog::load(Common::SeekableReadStream &rs, const Common::String &name) {
	clear();

	int64 size = rs.size();
	if (size < 28 || size > MO_MAX_SIZE) {
		warning("[U8-L10N] %s: not a translation catalog (size %d)", name.c_str(), (int)size);
		return false;
	}

	Common::Array<byte> data;
	data.resize(size);
	rs.seek(0);
	if (rs.read(data.data(), size) != (uint32)size) {
		warning("[U8-L10N] %s: read error", name.c_str());
		return false;
	}

	uint32 magic = READ_LE_UINT32(data.data());
	bool bigEndian;
	if (magic == MO_MAGIC)
		bigEndian = false;
	else if (magic == MO_MAGIC_SWAPPED)
		bigEndian = true;
	else {
		warning("[U8-L10N] %s: not a gettext MO file", name.c_str());
		return false;
	}

	auto read32 = [&](uint32 offset) -> uint32 {
		const byte *p = data.data() + offset;
		return bigEndian ? READ_BE_UINT32(p) : READ_LE_UINT32(p);
	};

	uint32 revision = read32(4);
	uint32 count = read32(8);
	uint32 origTable = read32(12);
	uint32 transTable = read32(16);
	if ((revision >> 16) > 1) {
		warning("[U8-L10N] %s: unsupported MO revision %x", name.c_str(), revision);
		return false;
	}
	// Each table has count (length, offset) pairs of 8 bytes
	if (count > (uint32)size / 16 ||
	        origTable > (uint32)size || (uint32)size - origTable < count * 8 ||
	        transTable > (uint32)size || (uint32)size - transTable < count * 8) {
		warning("[U8-L10N] %s: corrupt string tables", name.c_str());
		return false;
	}

	// Validate every string before using any of them
	auto stringOk = [&](uint32 table, uint32 index) -> bool {
		uint32 len = read32(table + index * 8);
		uint32 off = read32(table + index * 8 + 4);
		return off <= (uint32)size && len < (uint32)size - off;  // room for the NUL
	};
	auto getString = [&](uint32 table, uint32 index) -> Common::String {
		uint32 len = read32(table + index * 8);
		uint32 off = read32(table + index * 8 + 4);
		return Common::String(reinterpret_cast<const char *>(data.data()) + off, len);
	};

	for (uint32 i = 0; i < count; i++) {
		if (!stringOk(origTable, i) || !stringOk(transTable, i)) {
			warning("[U8-L10N] %s: string %u is out of bounds", name.c_str(), i);
			return false;
		}
	}

	for (uint32 i = 0; i < count; i++) {
		Common::String key = getString(origTable, i);
		Common::String translation = getString(transTable, i);
		if (key.empty())
			parseHeader(translation);
		else
			addEntry(key, translation, name);
	}

	debugC(1, kDebugLocalization, "[U8-L10N] %s: %u entries loaded, %u skipped, language \"%s\"",
	       name.c_str(), _entries.size(), _skipped, _language.c_str());
	return true;
}

bool TranslationCatalog::findContext(const Common::String &context, Common::String &source,
                                     Common::String &translation) const {
	Common::String canonical;
	if (!canonicalContext(context, canonical))
		return false;
	canonical += CONTEXT_SEPARATOR;
	for (const auto &e : _entries) {
		if (e._key.hasPrefix(canonical)) {
			source = e._key.substr(canonical.size());
			translation = e._value;
			return true;
		}
	}
	return false;
}

TranslationCatalog::Result TranslationCatalog::lookup(const Common::String &context,
		const Common::String &source, const Common::String *&translation) const {
	translation = nullptr;

	Common::String key = context;
	key += CONTEXT_SEPARATOR;
	key += source;
	EntryMap::const_iterator it = _entries.find(key);
	if (it != _entries.end()) {
		translation = &it->_value;
		return kHit;
	}
	return _barkContexts.contains(context) ? kSourceMismatch : kMiss;
}

} // End of namespace Ultima8
} // End of namespace Ultima
