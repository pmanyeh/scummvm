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

#include "ultima/ultima8/misc/options_widget.h"
#include "common/algorithm.h"
#include "common/archive.h"
#include "common/config-manager.h"
#include "common/fs.h"
#include "common/translation.h"
#include "common/util.h"
#include "gui/ThemeEval.h"
#include "gui/widgets/popup.h"

namespace Ultima {
namespace Ultima8 {

enum {
	kNumberChangedCmd = 'U8NC',
	kDefaultsCmd = 'U8DF'
};

// Keys written together: books and scrolls, gravestones and plaques
static const char *secondKey(const char *key) {
	static const struct {
		const char *_first, *_second;
	} pairs[] = {
		{ "font_cjk_book_file", "font_cjk_scroll_file" },
		{ "font_cjk_book_size", "font_cjk_scroll_size" },
		{ "font_cjk_book_letter_spacing", "font_cjk_scroll_letter_spacing" },
		{ "font_cjk_grave_file", "font_cjk_plaque_file" },
		{ "font_cjk_grave_size", "font_cjk_plaque_size" },
		{ "font_cjk_grave_letter_spacing", "font_cjk_plaque_letter_spacing" }
	};
	for (const auto &p : pairs) {
		if (!strcmp(p._first, key))
			return p._second;
	}
	return nullptr;
}

static Common::String numberString(double v, int scale) {
	if (scale == 1)
		return Common::String::format("%d", (int)(v + (v < 0 ? -0.5 : 0.5)));
	Common::String s = Common::String::format("%.1f", v);
	if (s.hasSuffix(".0"))
		s.erase(s.size() - 2);
	if (s == "-0")
		s = "0";
	return s;
}

OptionsWidget::OptionsWidget(GuiObject *boss, const Common::String &name, const Common::String &domain,
		const ExtraGuiOptions &options) :
		OptionsContainerWidget(boss, name, "Ultima8OptionsDialog", domain), _options(options) {
	// the engine's checkboxes, as in GUI::ExtraGuiOptionsWidget
	for (uint i = 0; i < _options.size(); i++) {
		Common::String id = Common::String::format("customOption%dCheckbox", i + 1);
		_optionCheckboxes.push_back(new GUI::CheckboxWidget(widgetsBoss(), _dialogLayout + "." + id,
		                            _(_options[i].label), _(_options[i].tooltip)));
		_layoutRows.push_back("c:" + id);
	}

	findFonts();

	addHeader("HdHeader", _("Dialogue text (high-resolution)"));
	addFont("HdFont", _("Font:"), _("The font of conversations, barks and answers with the high-resolution Chinese text."),
	        "font_cjk_hd_file", "jf-openhuninn-2.1.ttf", Common::U32String("jf-openhuninn-2.1.ttf"));
	addNumber("HdSize", _("Size:"), _("Font size, in game pixels (the screen is 320 wide)."),
	          "font_cjk_hd_size", 1, 8, 24, 12);
	addNumber("HdLetter", _("Letter spacing:"), _("Extra space between characters, in game pixels."),
	          "font_cjk_hd_letter_spacing", 10, -1, 3, 0.3);
	addNumber("HdLatin", _("Chinese-Latin spacing:"), _("Extra space where Chinese characters meet Latin letters or digits, in game pixels."),
	          "font_cjk_hd_latin_spacing", 10, 0, 5, 1);
	addNumber("HdLine", _("Line spacing:"), _("Extra space between lines, in game pixels."),
	          "font_cjk_hd_line_spacing", 10, -2, 8, 0);
	addNumber("HdBorder", _("Outline:"), _("Thickness of the dark outline of the text, in game pixels. 0: none."),
	          "font_cjk_hd_border", 10, 0, 2, 0.5);

	addHeader("PxHeader", _("Dialogue text (pixel font)"));
	addFont("PxFont", _("Font:"), _("The font used when the high-resolution Chinese text is off."),
	        "font_cjk_file", "Cubic_11.ttf", Common::U32String("Cubic_11.ttf"));
	addNumber("PxSize", _("Size:"), _("Font size, in game pixels."), "font_cjk_size", 1, 8, 20, 12);
	addNumber("PxLetter", _("Letter spacing:"), _("Extra space between characters, in game pixels."),
	          "font_cjk_letter_spacing", 1, -2, 6, 1);
	addNumber("PxLatin", _("Chinese-Latin spacing:"), _("Extra space where Chinese characters meet Latin letters or digits, in game pixels."),
	          "font_cjk_latin_spacing", 1, 0, 8, 3);
	addNumber("PxLine", _("Line spacing:"), _("Extra space between lines, in game pixels."),
	          "font_cjk_line_spacing", 1, -2, 8, 0);

	addHeader("BarkHeader", _("Speech bubbles"));
	addNumber("BarkLines", _("Lines per page:"), _("Lines of Chinese text in a speech bubble before the next page."),
	          "bark_lines", 1, 2, 8, 4);
	addNumber("BarkWidth", _("Width:"), _("Width of a speech bubble with Chinese text, in game pixels."),
	          "bark_width", 1, 120, 300, 194);

	addHeader("BookHeader", _("Books and scrolls"));
	addFont("BookFont", _("Font:"), _("The font of books and scrolls."),
	        "font_cjk_book_file", "", _("Same as the dialogue"));
	addNumber("BookSize", _("Size:"), _("Font size of books and scrolls, in game pixels."),
	          "font_cjk_book_size", 1, 8, 24, 12);
	addNumber("BookLetter", _("Letter spacing:"), _("Extra space between characters, in game pixels."),
	          "font_cjk_book_letter_spacing", 10, -1, 3, 0.3);

	addHeader("EngraveHeader", _("Gravestones and plaques"));
	addFont("EngraveFont", _("Font:"), _("The font of the Chinese engravings."),
	        "font_cjk_grave_file", "WangHanZongWeiBeiTiFan-2.ttf", Common::U32String("WangHanZongWeiBeiTiFan-2.ttf"));
	addNumber("EngraveSize", _("Size:"), _("Font size of the engravings, in game pixels."),
	          "font_cjk_grave_size", 1, 12, 48, 32);
	addNumber("EngraveLetter", _("Letter spacing:"), _("Extra space between characters, in game pixels."),
	          "font_cjk_grave_letter_spacing", 10, -1, 3, 0.3);

	addHeader("ControlHeader", _("Controls"));
	addBool("ClickAnywhere", _("Click anywhere for the next page of speech"),
	        _("In a conversation, a click anywhere goes to the next page, as in Ultima VII. Off: click the text."),
	        "bark_click_anywhere", true);
	addBool("EscCloses", _("Escape closes the frontmost window first"),
	        _("Escape closes the frontmost container or inventory before it opens the menu."),
	        "esc_closes_gumps", true);
	addBool("AssistedJump", _("Land targeted jumps on the middle of the target"),
	        _("With jump to mouse position, the Avatar lands with his whole footing on the stone or platform under the mouse, and shakes his head if there is no room. Off: the original landing."),
	        "assisted_jump", true);
	addBool("AutoHide", _("Show the quick action icons only under the mouse"),
	        _("Off: the quick action icons are always shown."),
	        "quick_bar_autohide", true);

	new GUI::ButtonWidget(widgetsBoss(), _dialogLayout + ".Defaults", _("Defaults"),
	                      _("Set the settings above (not the checkboxes) back to their defaults."), kDefaultsCmd);
	_layoutRows.push_back("b:Defaults");
}

OptionsWidget::~OptionsWidget() {
}

void OptionsWidget::addHeader(const char *id, const Common::U32String &text) {
	GUI::StaticTextWidget *t = new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + "." + id, text);
	t->setAlign(Graphics::kTextAlignStart);
	_layoutRows.push_back(Common::String("h:") + id);
}

void OptionsWidget::addFont(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
		const char *key, const char *def, const Common::U32String &defaultName) {
	GUI::StaticTextWidget *t = new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + "." + id + "Label", label);
	t->setAlign(Graphics::kTextAlignEnd);
	GUI::PopUpWidget *popup = new GUI::PopUpWidget(widgetsBoss(), _dialogLayout + "." + id, tooltip);
	popup->appendEntry(Common::U32String::format(_("Default (%S)"), defaultName.c_str()), 0);
	for (uint i = 0; i < _fontFiles.size(); i++)
		popup->appendEntry(Common::U32String(_fontFiles[i]), i + 1);
	FontChoice f = { key, def, popup };
	_fonts.push_back(f);
	_layoutRows.push_back(Common::String("f:") + id);
}

void OptionsWidget::addNumber(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
		const char *key, int scale, int min, int max, double def) {
	GUI::StaticTextWidget *t = new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + "." + id + "Label", label);
	t->setAlign(Graphics::kTextAlignEnd);
	GUI::SliderWidget *slider = new GUI::SliderWidget(widgetsBoss(), _dialogLayout + "." + id, tooltip, kNumberChangedCmd);
	slider->setMinValue(min * scale);
	slider->setMaxValue(max * scale);
	GUI::StaticTextWidget *value = new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + "." + id + "Value", Common::U32String());
	value->setFlags(GUI::WIDGET_CLEARBG);
	NumberChoice n = { key, scale, min, max, def, slider, value };
	_numbers.push_back(n);
	_layoutRows.push_back(Common::String("n:") + id);
}

void OptionsWidget::addBool(const char *id, const Common::U32String &label, const Common::U32String &tooltip,
		const char *key, bool def) {
	GUI::CheckboxWidget *c = new GUI::CheckboxWidget(widgetsBoss(), _dialogLayout + "." + id, label, tooltip);
	BoolChoice b = { key, def, c };
	_bools.push_back(b);
	_layoutRows.push_back(Common::String("c:") + id);
}

// The fonts next to the translation: the extra path, the game's folder
// and, on macOS, the app's resources
void OptionsWidget::findFonts() {
	Common::Array<Common::FSNode> dirs;
	if (ConfMan.hasKey("extrapath", _domain))
		dirs.push_back(Common::FSNode(ConfMan.getPath("extrapath", _domain)));
	// also --extrapath on the command line (the transient domain)
	if (ConfMan.hasKey("extrapath"))
		dirs.push_back(Common::FSNode(ConfMan.getPath("extrapath")));
	if (ConfMan.hasKey("path", _domain))
		dirs.push_back(Common::FSNode(ConfMan.getPath("path", _domain)));

	Common::ArchiveMemberList members;
	for (const Common::FSNode &dir : dirs) {
		if (!dir.isDirectory())
			continue;
		Common::FSDirectory fsdir(dir);
		fsdir.listMatchingMembers(members, "*.ttf");
		fsdir.listMatchingMembers(members, "*.otf");
	}
	Common::Archive *bundle = SearchMan.getArchive("__OSX_BUNDLE__");
	if (bundle) {
		bundle->listMatchingMembers(members, "*.ttf");
		bundle->listMatchingMembers(members, "*.otf");
	}

	for (const Common::ArchiveMemberPtr &m : members) {
		Common::String file = m->getFileName();
		bool known = false;
		for (const Common::String &f : _fontFiles)
			known = known || f.equalsIgnoreCase(file);
		if (!known)
			_fontFiles.push_back(file);
	}
	Common::sort(_fontFiles.begin(), _fontFiles.end());
}

Common::String OptionsWidget::inherited(const char *key, const Common::String &def) const {
	if (ConfMan.hasKey(key, Common::ConfigManager::kApplicationDomain) &&
	        !ConfMan.get(key, Common::ConfigManager::kApplicationDomain).empty())
		return ConfMan.get(key, Common::ConfigManager::kApplicationDomain);
	return def;
}

Common::String OptionsWidget::current(const char *key, const Common::String &def) const {
	if (ConfMan.hasKey(key, _domain) && !ConfMan.get(key, _domain).empty())
		return ConfMan.get(key, _domain);
	return inherited(key, def);
}

// Write a changed value to the game's section; the inherited value
// removes the key there
void OptionsWidget::store(const char *key, const Common::String &value, const Common::String &def) {
	const char *keys[2] = { key, secondKey(key) };
	for (const char *k : keys) {
		if (!k)
			continue;
		if (value == current(k, def))
			continue;
		if (value == inherited(k, def))
			ConfMan.removeKey(k, _domain);
		else
			ConfMan.set(k, value, _domain);
	}
}

void OptionsWidget::load() {
	for (uint j = 0; j < _options.size() && j < _optionCheckboxes.size(); ++j) {
		bool isChecked = _options[j].defaultState;
		if (ConfMan.hasKey(_options[j].configOption, _domain))
			isChecked = ConfMan.getBool(_options[j].configOption, _domain);
		_optionCheckboxes[j]->setState(isChecked);
	}

	for (FontChoice &f : _fonts) {
		Common::String file = current(f._key, f._default);
		uint32 tag = 0;
		for (uint i = 0; i < _fontFiles.size(); i++) {
			if (_fontFiles[i].equalsIgnoreCase(file) && !file.equalsIgnoreCase(f._default))
				tag = i + 1;
		}
		f._popup->setSelectedTag(tag);
	}

	for (NumberChoice &n : _numbers) {
		double v = atof(current(n._key, numberString(n._default, n._scale)).c_str());
		n._slider->setValue((int)(v * n._scale + (v < 0 ? -0.5 : 0.5)));
		updateValue(n);
	}

	for (BoolChoice &b : _bools) {
		Common::String v = current(b._key, b._default ? "true" : "false");
		bool state = b._default;
		Common::parseBool(v, state);
		b._checkbox->setState(state);
	}
}

bool OptionsWidget::save() {
	for (uint i = 0; i < _options.size() && i < _optionCheckboxes.size(); i++)
		ConfMan.setBool(_options[i].configOption, _optionCheckboxes[i]->getState(), _domain);

	for (const FontChoice &f : _fonts) {
		uint32 tag = f._popup->getSelectedTag();
		Common::String file = tag > 0 && tag <= _fontFiles.size() ? _fontFiles[tag - 1] : Common::String(f._default);
		store(f._key, file, f._default);
	}

	for (const NumberChoice &n : _numbers) {
		Common::String def = numberString(n._default, n._scale);
		store(n._key, numberString((double)n._slider->getValue() / n._scale, n._scale), def);
	}

	for (const BoolChoice &b : _bools)
		store(b._key, b._checkbox->getState() ? "true" : "false", b._default ? "true" : "false");

	return true;
}

void OptionsWidget::updateValue(NumberChoice &n) {
	n._value->setLabel(numberString((double)n._slider->getValue() / n._scale, n._scale));
}

void OptionsWidget::setDefaults() {
	for (FontChoice &f : _fonts)
		f._popup->setSelectedTag(0);
	for (NumberChoice &n : _numbers) {
		n._slider->setValue((int)(n._default * n._scale + (n._default < 0 ? -0.5 : 0.5)));
		updateValue(n);
	}
	for (BoolChoice &b : _bools)
		b._checkbox->setState(b._default);
	markAsDirty();
}

void OptionsWidget::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kNumberChangedCmd:
		for (NumberChoice &n : _numbers) {
			if (n._slider == sender) {
				updateValue(n);
				n._value->markAsDirty();
			}
		}
		break;
	case kDefaultsCmd:
		setDefaults();
		break;
	default:
		GUI::OptionsContainerWidget::handleCommand(sender, cmd, data);
		break;
	}
}

void OptionsWidget::defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName,
		const Common::String &overlayedLayout) const {
	layouts.addDialog(layoutName, overlayedLayout);
	layouts.addLayout(GUI::ThemeLayout::kLayoutVertical).addPadding(0, 0, 0, 0);

	for (const Common::String &row : _layoutRows) {
		Common::String kind = row.substr(0, 2);
		Common::String id = row.substr(2);
		if (kind == "c:") {
			layouts.addWidget(id, "Checkbox");
		} else if (kind == "h:") {
			layouts.addSpace(10);
			layouts.addWidget(id, "", -1, layouts.getVar("Globals.Line.Height"));
		} else if (kind == "f:") {
			layouts.addLayout(GUI::ThemeLayout::kLayoutHorizontal, 12, GUI::ThemeLayout::kItemAlignCenter)
				.addPadding(0, 0, 0, 0)
				.addWidget(id + "Label", "OptionsLabel")
				.addWidget(id, "PopUp")
				.closeLayout();
		} else if (kind == "n:") {
			layouts.addLayout(GUI::ThemeLayout::kLayoutHorizontal, 12, GUI::ThemeLayout::kItemAlignCenter)
				.addPadding(0, 0, 0, 0)
				.addWidget(id + "Label", "OptionsLabel")
				.addWidget(id, "Slider")
				.addWidget(id + "Value", "ShortOptionsLabel")
				.closeLayout();
		} else if (kind == "b:") {
			layouts.addSpace(10);
			layouts.addWidget(id, "Button");
		}
	}

	layouts.closeLayout().closeDialog();
}

} // End of namespace Ultima8
} // End of namespace Ultima
