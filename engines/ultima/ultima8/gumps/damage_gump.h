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

#ifndef ULTIMA8_GUMPS_DAMAGE_GUMP_H
#define ULTIMA8_GUMPS_DAMAGE_GUMP_H

#include "common/str.h"
#include "ultima/ultima8/gumps/item_relative_gump.h"
#include "ultima/ultima8/misc/classtype.h"

namespace Ultima {
namespace Ultima8 {

/**
 * The damage an actor takes, floating up from him for about a second ("-7",
 * or "Blocked" when the armour took it all), in another colour on the
 * Avatar; "Miss" over the Avatar when his blow hit nothing. Not saved.
 * Game option damage_numbers.
 */
class DamageGump : public ItemRelativeGump {
public:
	ENABLE_RUNTIME_CLASSTYPE()

	DamageGump(uint16 owner, const Common::String &text, unsigned int cjkFont);
	~DamageGump() override;

	void run() override;
	void Paint(RenderSurface *surf, int32 lerp_factor, bool scaled) override;

	//! Show the damage over an actor, if the option is on
	static void show(uint16 actor, int damage);
	//! Show that the Avatar's blow hit nothing, if the option is on
	static void showMiss(uint16 actor);

	//! Whether an actor is on the game map, to show something over him
	static bool isOnMap(uint16 actor);

private:
	DamageGump();
	int _age;   //!< frames shown
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
