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

#ifndef ULTIMA8_GUMPS_QUICKBARGUMP_H
#define ULTIMA8_GUMPS_QUICKBARGUMP_H

#include "ultima/ultima8/gumps/gump.h"
#include "ultima/ultima8/misc/classtype.h"

namespace Ultima {
namespace Ultima8 {

/**
 * Icons in a bottom corner, shown while the mouse is there, like the bars
 * of Exult (U8 only). Not saved: created again for every game.
 * - ACTIONS, bottom right: the actions of the keyboard shortcuts (backpack,
 *   keyring, save, ...); quick_bar setting.
 * - CHEATS, bottom left: the cheats of the console (max stats, heal,
 *   invincibility, hack mover); only while cheats are enabled.
 * quick_bar_autohide=false keeps them shown.
 */
class QuickBarGump : public Gump {
public:
	ENABLE_RUNTIME_CLASSTYPE()

	enum Kind {
		ACTIONS,
		CHEATS
	};

	QuickBarGump(Kind kind = ACTIONS);
	~QuickBarGump() override;

	void InitGump(Gump *newparent, bool take_focus = false) override;
	void run() override;

	void PaintThis(RenderSurface *surf, int32 lerp_factor, bool scaled) override;

	bool PointOnGump(int mx, int my) override;
	uint16 TraceObjId(int32 mx, int32 my) override;
	Gump *onMouseDown(int button, int32 mx, int32 my) override;
	void onMouseUp(int button, int32 mx, int32 my) override;
	Gump *onMouseMotion(int32 mx, int32 my) override;
	void onMouseLeft() override;

	//! Create or close the bars as the settings say
	static void updateBars();

private:
	//! The bar of this kind on the desktop, or nullptr
	static QuickBarGump *find(Kind kind);
	//! True if the bar can be used now (the cheats bar needs cheats)
	bool isAvailable() const;
	//! Icon under the point (gump coordinates), or -1
	int iconAt(int32 x, int32 y) const;
	void paintIcon(RenderSurface *surf, int index, const Common::Rect32 &cell);
	void doIcon(int index);
	void setHover(int index);
	//! Keep the bar in its corner, next to the bars of health and mana
	void updatePosition();

	static uint16 _barIds[2]; //!< object ids of the bars, by kind

	Kind _kind;
	int _columns, _rows;
	int32 _cellsX; //!< left of the icons; the rest of the width is for the tooltip
	bool _shown;
	int _hover;
	int _pressed;
	uint16 _tooltipId;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
