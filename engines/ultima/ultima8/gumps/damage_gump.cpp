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
#include "ultima/ultima8/gumps/damage_gump.h"
#include "ultima/ultima8/gfx/fonts/font_manager.h"
#include "ultima/ultima8/gumps/widgets/text_widget.h"
#include "ultima/ultima8/misc/localization.h"
#include "ultima/ultima8/ultima8.h"

namespace Ultima {
namespace Ultima8 {

DEFINE_RUNTIME_CLASSTYPE_CODE(DamageGump)

// frames shown, and game pixels risen per frame
static const int DAMAGE_FRAMES = 36;

DamageGump::DamageGump() : ItemRelativeGump(), _age(0) {
}

DamageGump::DamageGump(uint16 owner, const Common::String &text) :
		ItemRelativeGump(0, 0, 0, 0, owner, FLAG_DONT_SAVE, LAYER_NORMAL), _age(0) {
	// the font of the damage numbers (font_cjk_damage_*), or a game font
	unsigned int font = FontManager::get_instance()->localizedFont(0, FontManager::CJK_DAMAGE_FONT);
	TextWidget *widget = new TextWidget(0, 0, text, true, font, 0, 0, Font::TEXT_CENTER);
	widget->InitGump(this, false);
	_dims = widget->getDims();
}

DamageGump::~DamageGump() {
}

void DamageGump::run() {
	ItemRelativeGump::run();
	if (++_age >= DAMAGE_FRAMES)
		Close();
}

void DamageGump::Paint(RenderSurface *surf, int32 lerp_factor, bool scaled) {
	GetItemLocation(lerp_factor);
	// float up from above the head
	_iy -= 4 + _age / 2;
	Gump::Paint(surf, lerp_factor, scaled);
}

void DamageGump::show(uint16 actor, int damage) {
	if (!GAME_IS_U8 || !ConfMan.getBool("damage_numbers"))
		return;
	Common::String text;
	if (damage > 0) {
		text = Common::String::format("-%d", damage);
	} else {
		Localization *l10n = Localization::get_instance();
		text = l10n && l10n->isActive() ? "\xE6\xA0\xBC\xE6\x93\x8B" : "Blocked"; // 格擋
	}
	Gump *gump = new DamageGump(actor, text);
	gump->InitGump(nullptr, false);
}

} // End of namespace Ultima8
} // End of namespace Ultima
