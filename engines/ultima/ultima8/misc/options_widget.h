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

#ifndef ULTIMA8_MISC_OPTIONS_WIDGET_H
#define ULTIMA8_MISC_OPTIONS_WIDGET_H

#include "common/array.h"
#include "common/str-array.h"
#include "engines/metaengine.h"
#include "gui/widget.h"

namespace GUI {
class CheckboxWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
}

namespace Ultima {
namespace Ultima8 {

/**
 * The game options of Ultima VIII: the engine's checkboxes, then the
 * settings of the Chinese text (fonts, sizes, spacing of the dialogue,
 * books and engravings, the speech bubbles) and of the controls, which
 * were only in scummvm.ini before. Values equal to the inherited ones
 * (the [scummvm] section or the engine default) are not written to the
 * game's section.
 */
class OptionsWidget : public GUI::OptionsContainerWidget {
public:
	OptionsWidget(GuiObject *boss, const Common::String &name, const Common::String &domain,
	              const ExtraGuiOptions &options);
	~OptionsWidget() override;

	void load() override;
	bool save() override;
	void handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) override;

protected:
	void defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName,
	                  const Common::String &overlayedLayout) const override;

private:
	struct FontChoice {
		const char *_key;
		const char *_default;   //!< the file used when not set ("" for none)
		GUI::PopUpWidget *_popup;
	};
	struct NumberChoice {
		const char *_key;
		int _scale;             //!< 10: tenths
		int _min, _max;         //!< slider range, in units / _scale
		double _default;
		GUI::SliderWidget *_slider;
		GUI::StaticTextWidget *_value;
	};
	struct BoolChoice {
		const char *_key;
		bool _default;
		GUI::CheckboxWidget *_checkbox;
	};

	ExtraGuiOptions _options;
	Common::Array<GUI::CheckboxWidget *> _optionCheckboxes;
	Common::Array<FontChoice> _fonts;
	Common::Array<NumberChoice> _numbers;
	Common::Array<BoolChoice> _bools;
	Common::StringArray _fontFiles;   //!< popup tag - 1 = index

	void addHeader(const char *id, const Common::U32String &text);
	void addFont(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
	             const char *key, const char *def, const Common::U32String &defaultName);
	void addNumber(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
	               const char *key, int scale, int min, int max, double def);
	void addBool(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
	             const char *key, bool def);

	//! Value inherited by the game: the [scummvm] section, or the default
	Common::String inherited(const char *key, const Common::String &def) const;
	Common::String current(const char *key, const Common::String &def) const;
	void store(const char *key, const Common::String &value, const Common::String &def);
	void updateValue(NumberChoice &n);
	void findFonts();
	void setDefaults();

	Common::Array<Common::String> _layoutRows;   //!< ids in layout order; "h:" headers, "c:" checkboxes, "r:" rows
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
