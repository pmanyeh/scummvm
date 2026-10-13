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
#include "ultima/ultima8/world/item.h"
#include "ultima/ultima8/games/game_data.h"
#include "ultima/ultima8/gfx/fonts/font.h"
#include "ultima/ultima8/gfx/fonts/font_manager.h"
#include "ultima/ultima8/kernel/kernel.h"
#include "ultima/ultima8/kernel/process.h"
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

// Lay out text the way a paging TextWidget does; returns the page count
uint layoutPages(Font *font, const Common::String &text, int32 width, int32 height,
                 int32 &maxWidth, int32 &maxHeight) {
	uint pages = 0;
	uint start = 0;
	maxWidth = maxHeight = 0;
	while (start < text.size()) {
		int32 w = 0, h = 0;
		unsigned int remaining = 0;
		font->getTextSize(text.substr(start), w, h, remaining, width, height,
		                  Font::TEXT_LEFT, true, true);
		pages++;
		maxWidth = MAX(maxWidth, w);
		maxHeight = MAX(maxHeight, h);
		if (!remaining)
			break;
		start += remaining;
	}
	return pages;
}

} // End of anonymous namespace

Localization *Localization::_instance = nullptr;

Localization::Localization() : _catalogLoaded(false), _fontReady(false), _annotate(true) {
	_instance = this;

	ConfMan.registerDefault("localization", "off");
	ConfMan.registerDefault("localization_measure", false);
	ConfMan.registerDefault("localization_annotate", true);
}

Localization::~Localization() {
	assert(_instance == this);
	_instance = nullptr;
}

void Localization::configure(bool supportedGame) {
	// The game option checkbox (localization_zh_tw) decides once it has
	// been set, else the localization setting; with neither, this
	// Traditional Chinese edition of the engine shows Chinese (the
	// checkbox is checked by default)
	Common::String language;
	bool requested = true;
	if (ConfMan.hasKey("localization_zh_tw")) {
		language = ConfMan.getBool("localization_zh_tw") ? "zh_TW" : "off";
	} else if (ConfMan.hasKey("localization")) {
		language = ConfMan.get("localization");
	} else {
		language = "zh_TW";
		requested = false;
	}
	language.trim();
	if (language.empty() || language.equalsIgnoreCase("off"))
		language.clear();

	if (!language.empty() && !supportedGame) {
		// only an explicit setting is worth a warning
		if (requested)
			warning("[U8-L10N] localization %s is only available for the English version of Ultima VIII",
			        language.c_str());
		language.clear();
	}

	_annotate = ConfMan.getBool("localization_annotate");
	_shownTerms.clear();

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

Common::String Localization::translateBark(const Common::String &source, const Item *speaker) const {
	// Inside an intrinsic the IP of the running process still points at
	// the call instruction, which identifies the bark.
	UCProcess *proc = dynamic_cast<UCProcess *>(Kernel::get_instance()->getRunningProcess());
	if (!proc) {
		debugC(1, kDebugLocalization, "[U8-L10N] bark without usecode process src=\"%s\"", source.c_str());
		return source;
	}
	Common::String context = TranslationCatalog::barkContext(proc->getClassId(), proc->getIp());
	Common::String translation = source;
	Common::String frameContext = speaker ? context + Common::String::format("#%u", speaker->getFrame()) : "";
	if (speaker && isActive() && _catalog.lookup(frameContext, source, translation) == TranslationCatalog::kHit)
		debugC(1, kDebugLocalization, "[U8-L10N] %s HIT src=\"%s\"", frameContext.c_str(), source.c_str());
	else
		translation = translate(context, source);
	if (translation != source)
		translation = annotate(translation, proc);
	return translation;
}

Common::String Localization::annotate(const Common::String &text, Process *conversation) const {
	if (!_annotate)
		return text;
	if (!conversation) {
		Common::HashMap<Common::String, bool> shown;
		return _catalog.annotateTerms(text, shown);
	}

	// A conversation is one usecode process: forget the names shown in
	// processes that have ended
	Kernel *kernel = Kernel::get_instance();
	for (uint i = 0; i < _shownTerms.size();) {
		Process *p = kernel->getProcess(_shownTerms[i]._pid);
		if (p != _shownTerms[i]._process || p->is_terminated())
			_shownTerms.remove_at(i);
		else
			i++;
	}
	ShownTerms *entry = nullptr;
	for (ShownTerms &s : _shownTerms) {
		if (s._process == conversation)
			entry = &s;
	}
	if (!entry) {
		ShownTerms s;
		s._pid = conversation->getPid();
		s._process = conversation;
		_shownTerms.push_back(s);
		entry = &_shownTerms.back();
	}
	return _catalog.annotateTerms(text, entry->_shown);
}

Common::String Localization::translateCallSite(const char *kind, const Common::String &source,
                                               int fontnum) const {
	if (!isActive())
		return source;
	UCProcess *proc = dynamic_cast<UCProcess *>(Kernel::get_instance()->getRunningProcess());
	if (!proc)
		return source;
	Common::String translation = translate(
		TranslationCatalog::callSiteContext(kind, proc->getClassId(), proc->getIp()), source);
	if (translation != source) {
		Font *font = FontManager::get_instance()->getGameFont(fontnum, true);
		if (!font || !font->isUTF8()) {
			warning("[U8-L10N] font %d cannot show localized text, showing English", fontnum);
			return source;
		}
		// each book, scroll, ... introduces its names again
		translation = annotate(translation, nullptr);
	}
	return translation;
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

void Localization::measureCatalog() const {
	if (!isActive()) {
		debugC(1, kDebugLocalization, "[U8-L10N] measure: localization is not active");
		return;
	}

	// Text box of each kind of text: font, width, height (0 = unlimited)
	struct Box {
		const char *_kind;
		int _font;
		int32 _width, _height;
	};
	static const Box boxes[] = {
		{ "bark", 0, 194, 55 },     // BarkGump (fonts 0, 5-8 are the same CJK font)
		{ "ask", 0, 0, 0 },         // AskGump answer, one line ("@ " + text)
		{ "book", 9, 123, 129 },    // BookGump, one page (two per spread)
		{ "scroll", 9, 204, 115 },  // ScrollGump
		{ "grave", 6, 0, 0 },       // ReadableGump subtitle (one line)
		{ "plaque", 6, 0, 0 }
	};

	Common::Array<TranslationCatalog::Entry> entries;
	_catalog.listEntries(entries);
	FontManager *fm = FontManager::get_instance();
	debugC(1, kDebugLocalization, "[U8-L10N] measure\tcontext\tpagesEN\tpagesL10N\twEN\thEN"
	       "\twL10N\thL10N\treadEN\treadL10N\tsource");
	for (const TranslationCatalog::Entry &e : entries) {
		const Box *box = nullptr;
		for (const Box &b : boxes) {
			if (e._context.hasPrefix(b._kind) && e._context[strlen(b._kind)] == ' ')
				box = &b;
		}
		if (!box)
			continue;

		Common::String source = e._source, translation = e._translation;
		if (!strcmp(box->_kind, "ask")) {
			source = "@ " + source;
			translation = "@ " + translation;
		} else {
			// as if each name in it were shown for the first time
			translation = annotate(translation, nullptr);
		}
		Font *original = fm->getGameFont(box->_font, false);
		Font *localized = fm->getGameFont(box->_font, true);
		int32 wEN = 0, hEN = 0, wL = 0, hL = 0;
		// gravestone and plaque: the English engraving stays, only the
		// subtitle is new
		bool subtitle = !strcmp(box->_kind, "grave") || !strcmp(box->_kind, "plaque");
		uint pagesEN = subtitle ? 0 : layoutPages(original, source, box->_width, box->_height, wEN, hEN);
		uint pagesL = layoutPages(localized, translation, box->_width, box->_height, wL, hL);

		Common::String shown = e._source;
		for (uint i = 0; i < shown.size(); i++) {
			if (shown[i] == '\t' || shown[i] == '\n')
				shown.setChar(' ', i);
		}
		debugC(1, kDebugLocalization, "[U8-L10N] measure\t%s\t%u\t%u\t%d\t%d\t%d\t%d\t%u\t%u\t%s",
		       e._context.c_str(), pagesEN, pagesL, wEN, hEN, wL, hL,
		       TranslationCatalog::readingLength(e._source),
		       TranslationCatalog::readingLength(e._translation), shown.c_str());
	}
	debugC(1, kDebugLocalization, "[U8-L10N] measure done, %u entries", entries.size());
}

Common::String Localization::dumpInfo() const {
	if (_language.empty())
		return "Localization: off\n";

	Common::String info = Common::String::format("Localization: %s (%s)\n", _language.c_str(),
	                      isActive() ? "active" : "inactive, showing English");
	info += Common::String::format("Catalog: %s, %s", _file.c_str(),
	                               _catalogLoaded ? "loaded" : "not loaded");
	if (_catalogLoaded)
		info += Common::String::format(", %u entries, %u skipped, %u names (%s)", _catalog.size(),
		                               _catalog.skipped(), _catalog.termCount(),
		                               _annotate ? "annotated" : "not annotated");
	info += Common::String::format("\nCJK font: %s\n", _fontReady ? "ready" : "not loaded");
	return info;
}

} // End of namespace Ultima8
} // End of namespace Ultima
