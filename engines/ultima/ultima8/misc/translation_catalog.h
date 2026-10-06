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

#ifndef ULTIMA8_MISC_TRANSLATION_CATALOG_H
#define ULTIMA8_MISC_TRANSLATION_CATALOG_H

#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/str.h"

namespace Common {
class SeekableReadStream;
}

namespace Ultima {
namespace Ultima8 {

/**
 * Translations of the game text, compiled from PO files into one gettext MO
 * file. Every entry has a context that identifies where the text is shown:
 *
 *   bark CCCC:IIII   NPC speech: usecode class and IP of the bark intrinsic call
 *   ask CCCC         conversation answer: usecode class that called ask
 *
 * (hex, case-insensitive in the file). The msgid is the exact English text,
 * including trailing spaces; an entry is only used when it matches the text
 * the game is about to show.
 */
class TranslationCatalog {
public:
	enum Result {
		kMiss,           //!< no entry (for an answer: no entry for this text)
		kHit,            //!< translation found
		kSourceMismatch  //!< bark call site known, but not with this English text
	};

	TranslationCatalog();

	void clear();

	/**
	 * Load a compiled catalog. Entries that are malformed (unknown context,
	 * empty or invalid UTF-8 translation, plurals, duplicates) are skipped.
	 * If the file structure itself is broken nothing is loaded.
	 * \return true if the file could be read
	 */
	bool load(Common::SeekableReadStream &rs, const Common::String &name);

	//! Look up a translation. translation is set only on kHit.
	Result lookup(const Common::String &context, const Common::String &source,
	              const Common::String *&translation) const;

	//! Any one entry of a context (for testing); false if there is none
	bool findContext(const Common::String &context, Common::String &source,
	                 Common::String &translation) const;

	uint size() const { return _entries.size(); }
	uint skipped() const { return _skipped; }
	//! Language from the catalog header (may be empty)
	const Common::String &language() const { return _language; }

	static Common::String barkContext(uint16 classId, uint16 ip);
	static Common::String askContext(uint16 classId);

	//! Parse a context and write it in canonical form ("bark 0402:0633")
	static bool canonicalContext(const Common::String &context, Common::String &canonical);

	//! True if the whole string is valid UTF-8
	static bool isValidUTF8(const Common::String &text);

private:
	typedef Common::HashMap<Common::String, Common::String> EntryMap;
	typedef Common::HashMap<Common::String, bool> ContextSet;

	void addEntry(const Common::String &key, const Common::String &translation,
	              const Common::String &name);
	void parseHeader(const Common::String &header);

	EntryMap _entries;     //!< "<context>\x04<English>" -> translation
	ContextSet _barkContexts;  //!< bark call sites with at least one entry
	uint _skipped;
	Common::String _language;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
