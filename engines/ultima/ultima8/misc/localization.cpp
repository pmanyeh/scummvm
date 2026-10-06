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

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/textconsole.h"

#include "ultima/ultima.h"
#include "ultima/ultima8/misc/localization.h"
#include "ultima/ultima8/games/game_data.h"
#include "ultima/ultima8/gfx/fonts/font.h"
#include "ultima/ultima8/gfx/fonts/font_manager.h"
#include "ultima/ultima8/kernel/kernel.h"
#include "ultima/ultima8/usecode/uc_process.h"

namespace Ultima {
namespace Ultima8 {

namespace {

// "zh-TW" and "zh_tw" name the same language
Common::String normalizeLanguage(Common::String lang) {
	lang.trim();
	lang.toLowercase();
	lang.replace('-', '_');
	return lang;
}

} // End of anonymous namespace

Localization *Localization::_instance = nullptr;

Localization::Localization() : _catalogLoaded(false), _fontReady(false) {
	_instance = this;

	ConfMan.registerDefault("localization", "off");
}

Localization::~Localization() {
	assert(_instance == this);
	_instance = nullptr;
}

void Localization::configure(bool supportedGame) {
	Common::String language = ConfMan.get("localization");
	language.trim();
	if (language.empty() || language.equalsIgnoreCase("off"))
		language.clear();

	if (!language.empty() && !supportedGame) {
		warning("[U8-L10N] localization %s is only available for the English version of Ultima VIII",
		        language.c_str());
		language.clear();
	}

	Common::String file;
	if (!language.empty()) {
		file = ConfMan.get("localization_file");
		if (file.empty())
			file = "u8_" + language + ".mo";
	}

	if (language == _language && file == _file)
		return;

	_language = language;
	_file = file;
	_catalog.clear();
	_catalogLoaded = false;
	_fontReady = false;

	if (_language.empty())
		return;

	Common::File f;
	if (!f.open(Common::Path(_file))) {
		warning("[U8-L10N] translation catalog %s not found, showing English text", _file.c_str());
		return;
	}
	if (!_catalog.load(f, _file)) {
		warning("[U8-L10N] translation catalog %s is invalid, showing English text", _file.c_str());
		_catalog.clear();
		return;
	}
	if (!_catalog.language().empty() &&
	        normalizeLanguage(_catalog.language()) != normalizeLanguage(_language)) {
		warning("[U8-L10N] %s is a %s catalog, not %s; showing English text", _file.c_str(),
		        _catalog.language().c_str(), _language.c_str());
		_catalog.clear();
		return;
	}
	_catalogLoaded = true;
}

void Localization::setCJKFontReady(bool ready) {
	if (_catalogLoaded && !ready)
		warning("[U8-L10N] CJK font could not be loaded, localization disabled");
	bool wasActive = isActive();
	_fontReady = _catalogLoaded && ready;
	if (isActive() && !wasActive)
		debugC(1, kDebugLocalization, "[U8-L10N] localization %s active, %u entries from %s",
		       _language.c_str(), _catalog.size(), _file.c_str());
}

Common::String Localization::translate(const Common::String &context,
                                       const Common::String &source) const {
	if (!isActive())
		return source;

	Common::String translation;
	switch (_catalog.lookup(context, source, translation)) {
	case TranslationCatalog::kHit:
		debugC(1, kDebugLocalization, "[U8-L10N] %s HIT src=\"%s\"", context.c_str(), source.c_str());
		return translation;
	case TranslationCatalog::kSourceMismatch:
		debugC(1, kDebugLocalization, "[U8-L10N] %s SOURCE-MISMATCH src=\"%s\"", context.c_str(),
		       source.c_str());
		return source;
	default:
		debugC(1, kDebugLocalization, "[U8-L10N] %s MISS src=\"%s\"", context.c_str(), source.c_str());
		return source;
	}
}

Common::String Localization::translateBark(const Common::String &source) const {
	// Inside an intrinsic the IP of the running process still points at
	// the call instruction, which identifies the bark.
	UCProcess *proc = dynamic_cast<UCProcess *>(Kernel::get_instance()->getRunningProcess());
	if (!proc) {
		debugC(1, kDebugLocalization, "[U8-L10N] bark without usecode process src=\"%s\"", source.c_str());
		return source;
	}
	return translate(TranslationCatalog::barkContext(proc->getClassId(), proc->getIp()), source);
}

Common::String Localization::translateAnswer(uint16 classId, const Common::String &source) const {
	return translate(TranslationCatalog::askContext(classId), source);
}

Common::String Localization::translateUI(const Common::String &english, int fontnum) const {
	if (!isActive())
		return english;
	Font *font = FontManager::get_instance()->getGameFont(fontnum, true);
	if (!font || !font->isUTF8())
		return english;
	return translate("ui", english);
}

Common::String Localization::uiText(const Common::String &english, int fontnum) {
	Localization *l10n = get_instance();
	if (l10n) {
		Common::String text = l10n->translateUI(english, fontnum);
		if (text != english)
			return text;
	}
	return _TL_(english);
}

uint16 Localization::runningClassId() {
	UCProcess *proc = dynamic_cast<UCProcess *>(Kernel::get_instance()->getRunningProcess());
	return proc ? proc->getClassId() : 0;
}

Common::String Localization::dumpInfo() const {
	if (_language.empty())
		return "Localization: off\n";

	Common::String info = Common::String::format("Localization: %s (%s)\n", _language.c_str(),
	                      isActive() ? "active" : "inactive, showing English");
	info += Common::String::format("Catalog: %s, %s", _file.c_str(),
	                               _catalogLoaded ? "loaded" : "not loaded");
	if (_catalogLoaded)
		info += Common::String::format(", %u entries, %u skipped", _catalog.size(), _catalog.skipped());
	info += Common::String::format("\nCJK font: %s\n", _fontReady ? "ready" : "not loaded");
	return info;
}

} // End of namespace Ultima8
} // End of namespace Ultima
