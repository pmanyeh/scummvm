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

#ifndef ULTIMA8_GUMPS_HEALTH_BAR_GUMP_H
#define ULTIMA8_GUMPS_HEALTH_BAR_GUMP_H

#include "ultima/ultima8/gumps/item_relative_gump.h"
#include "ultima/ultima8/misc/classtype.h"

namespace Ultima {
namespace Ultima8 {

/**
 * The hit points left of an actor who was hit, as a bar over his head for a
 * few seconds; a new hit shows it longer. Not for the Avatar, not saved.
 * Game option health_bars.
 */
class HealthBarGump : public ItemRelativeGump {
public:
	ENABLE_RUNTIME_CLASSTYPE()

	HealthBarGump(uint16 owner);
	~HealthBarGump() override;

	void run() override;
	void Paint(RenderSurface *surf, int32 lerp_factor, bool scaled) override;
	void PaintThis(RenderSurface *surf, int32 lerp_factor, bool scaled) override;

	//! No notifier: a hit kills the processes of the actor (to stop what he
	//! does), which would close us at once; we close when he is gone
	void CreateNotifier() override {}

	//! Show the health bar of an actor who was hit, if the option is on
	static void show(uint16 actor);

private:
	HealthBarGump();
	int _age;   //!< frames shown since the last hit
	int _fullHP; //!< the hit points of a full bar
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
