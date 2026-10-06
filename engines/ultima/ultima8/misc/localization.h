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

#include "common/str.h"
#include "ultima/ultima8/misc/translation_catalog.h"

namespace Ultima {
namespace Ultima8 {

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
	//! Display text for a conversation answer
	Common::String translateAnswer(uint16 classId, const Common::String &source) const;

	//! Class of the running usecode process (0 if none)
	static uint16 runningClassId();

	Common::String dumpInfo() const;

private:
	static Localization *_instance;

	Common::String _language;   //!< requested language, empty when off
	Common::String _file;       //!< catalog file in use
	bool _catalogLoaded;
	bool _fontReady;
	TranslationCatalog _catalog;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
