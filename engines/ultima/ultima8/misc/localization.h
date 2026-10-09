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

#ifndef ULTIMA8_MISC_LOCALIZATION_H
#define ULTIMA8_MISC_LOCALIZATION_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/str.h"
#include "ultima/ultima8/misc/common_types.h"
#include "ultima/ultima8/misc/translation_catalog.h"

namespace Ultima {
namespace Ultima8 {

class Process;

/**
 * Display-time localization. The game always runs on its original English
 * strings (usecode compares them, speech and saves use them); only the text
 * that is drawn is replaced.
 *
 * Game-domain configuration:
 *   localization          off (default) or a language, e.g. zh_TW
 *   localization_file     compiled catalog (default u8_<language>.mo)
 *   font_cjk_file         UTF-8 font used while localized (default Cubic_11.ttf)
 *   font_cjk_size         point size (default 12)
 *   font_cjk_antialiasing default false (pixel font)
 *   font_cjk_letter_spacing extra pixels between characters (default 0)
 *   font_cjk_latin_spacing  extra pixels between a CJK character and a
 *                           Latin letter or digit (default 0)
 *   font_cjk_line_spacing   extra pixels between lines (default 0)
 *
 * Localization is only active for English Ultima VIII, with a catalog that
 * loaded and a CJK font that loaded for every overridden game font. In every
 * other case the original English text is shown.
 */
class Localization {
public:
	Localization();
	~Localization();

	static Localization *get_instance() { return _instance; }

	/**
	 * Read the configuration and (re)load the catalog if needed.
	 * \param supportedGame true for the English version of Ultima VIII
	 */
	void configure(bool supportedGame);

	//! True if the game fonts should be replaced with the CJK font
	bool wantsCJKFont() const { return _catalogLoaded; }
	//! Report whether the CJK font could be installed for all game fonts
	void setCJKFontReady(bool ready);

	bool isActive() const { return _catalogLoaded && _fontReady; }
	const Common::String &language() const { return _language; }
	const TranslationCatalog &catalog() const { return _catalog; }

	/**
	 * Display text for a source string: the translation, or the source
	 * itself when localization is off or there is no matching entry.
	 */
	Common::String translate(const Common::String &context, const Common::String &source) const;

	//! Display text for a bark; must be called from the bark intrinsic
	Common::String translateBark(const Common::String &source) const;

	/**
	 * Display text of a book, scroll, gravestone or plaque; must be called
	 * from its intrinsic. English if the given game font cannot draw UTF-8.
	 */
	Common::String translateCallSite(const char *kind, const Common::String &source, int fontnum) const;
	//! Display text for a conversation answer
	Common::String translateAnswer(uint16 classId, const Common::String &source) const;

	/**
	 * Display text for an engine string ("ui" context) drawn with the given
	 * game font. English if there is no translation or the font cannot
	 * draw UTF-8.
	 */
	Common::String translateUI(const Common::String &english, int fontnum) const;

	//! Engine text: the localized text if there is one, else the game's
	//! own translation (_TL_)
	static Common::String uiText(const Common::String &english, int fontnum);

	//! Class of the running usecode process (0 if none)
	static uint16 runningClassId();

	/**
	 * Add the English to the names text shows for the first time in this
	 * conversation (setting localization_annotate). A conversation is the
	 * usecode process that shows it; nullptr: text that stands on its own
	 * (a book, scroll, ...).
	 */
	Common::String annotate(const Common::String &text, Process *conversation) const;

	Common::String dumpInfo() const;

	/**
	 * Lay out every catalog entry with the fonts and text box sizes of
	 * its kind of text and log the page counts and sizes, English and
	 * localized (setting localization_measure; tools/catalog/measure.py
	 * summarizes the log).
	 */
	void measureCatalog() const;

private:
	static Localization *_instance;

	Common::String _language;   //!< requested language, empty when off
	Common::String _file;       //!< catalog file in use
	bool _catalogLoaded;
	bool _fontReady;
	bool _annotate;             //!< add the English to names (localization_annotate)
	TranslationCatalog _catalog;

	//! Names already annotated in a conversation (usecode process)
	struct ShownTerms {
		ProcId _pid;
		Process *_process;
		Common::HashMap<Common::String, bool> _shown;
	};
	mutable Common::Array<ShownTerms> _shownTerms;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
