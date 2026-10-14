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
#include "common/hashmap.h"
#include "ultima/ultima8/gumps/health_bar_gump.h"
#include "ultima/ultima8/gumps/damage_gump.h"
#include "ultima/ultima8/gfx/render_surface.h"
#include "ultima/ultima8/gfx/texture.h"
#include "ultima/ultima8/world/actors/actor.h"
#include "ultima/ultima8/world/get_object.h"
#include "ultima/ultima8/ultima8.h"

namespace Ultima {
namespace Ultima8 {

DEFINE_RUNTIME_CLASSTYPE_CODE(HealthBarGump)

// frames shown after a hit (about four seconds), and the size of the bar
static const int HEALTH_BAR_FRAMES = 144;
static const int HEALTH_BAR_WIDTH = 24;
static const int HEALTH_BAR_HEIGHT = 4;

// the bar shown over each actor
static Common::HashMap<uint16, ObjId> g_healthBars;

HealthBarGump::HealthBarGump() : ItemRelativeGump(), _age(0) {
}

HealthBarGump::HealthBarGump(uint16 owner) :
		ItemRelativeGump(0, 0, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, owner, FLAG_DONT_SAVE, LAYER_NORMAL), _age(0) {
}

HealthBarGump::~HealthBarGump() {
	if (g_healthBars.contains(_owner) && g_healthBars[_owner] == getObjId())
		g_healthBars.erase(_owner);
}

void HealthBarGump::run() {
	ItemRelativeGump::run();
	const Actor *actor = getActor(_owner);
	if (++_age >= HEALTH_BAR_FRAMES || !actor || actor->isDead() || !DamageGump::isOnMap(_owner))
		Close();
}

void HealthBarGump::Paint(RenderSurface *surf, int32 lerp_factor, bool scaled) {
	GetItemLocation(lerp_factor);
	// just above the head
	_iy -= 1;
	Gump::Paint(surf, lerp_factor, scaled);
}

void HealthBarGump::PaintThis(RenderSurface *surf, int32 lerp_factor, bool scaled) {
	const Actor *actor = getActor(_owner);
	if (!actor)
		return;
	int maxHP = actor->getMaxHP();
	int hp = MIN<int>(actor->getHP(), maxHP);
	if (maxHP <= 0)
		return;

	const int inner = HEALTH_BAR_WIDTH - 2;
	int filled = (hp * inner + maxHP - 1) / maxHP;
	uint32 colour = hp * 2 > maxHP ? TEX32_PACK_RGB(0x30, 0xD0, 0x30)
	                : hp * 4 > maxHP ? TEX32_PACK_RGB(0xE0, 0xC8, 0x20)
	                : TEX32_PACK_RGB(0xE0, 0x30, 0x20);
	surf->fill32(TEX32_PACK_RGB(0, 0, 0), 0, 0, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT);
	surf->fill32(TEX32_PACK_RGB(0x50, 0x10, 0x10), 1, 1, inner, HEALTH_BAR_HEIGHT - 2);
	if (filled > 0)
		surf->fill32(colour, 1, 1, filled, HEALTH_BAR_HEIGHT - 2);
}

void HealthBarGump::show(uint16 actor) {
	if (!GAME_IS_U8 || actor == kMainActorId || !ConfMan.getBool("health_bars") ||
	        !DamageGump::isOnMap(actor))
		return;

	// already shown: show it longer
	if (g_healthBars.contains(actor)) {
		HealthBarGump *bar = dynamic_cast<HealthBarGump *>(getGump(g_healthBars[actor]));
		if (bar && !bar->IsClosing()) {
			bar->_age = 0;
			return;
		}
	}

	HealthBarGump *bar = new HealthBarGump(actor);
	bar->InitGump(nullptr, false);
	g_healthBars[actor] = bar->getObjId();
}

} // End of namespace Ultima8
} // End of namespace Ultima
