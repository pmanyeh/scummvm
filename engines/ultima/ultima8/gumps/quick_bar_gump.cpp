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

#include "ultima/ultima8/gumps/quick_bar_gump.h"

#include "common/config-manager.h"
#include "ultima/ultima8/audio/audio_process.h"
#include "ultima/ultima8/games/game_data.h"
#include "ultima/ultima8/gfx/gump_shape_archive.h"
#include "ultima/ultima8/gfx/main_shape_archive.h"
#include "ultima/ultima8/gfx/render_surface.h"
#include "ultima/ultima8/gfx/shape.h"
#include "ultima/ultima8/gfx/shape_frame.h"
#include "ultima/ultima8/gfx/texture.h"
#include "ultima/ultima8/gumps/mini_stats_gump.h"
#include "ultima/ultima8/gumps/widgets/text_widget.h"
#include "ultima/ultima8/kernel/mouse.h"
#include "ultima/ultima8/misc/localization.h"
#include "ultima/ultima8/world/actors/main_actor.h"
#include "ultima/ultima8/world/get_object.h"
#include "ultima/ultima8/ultima8.h"

namespace Ultima {
namespace Ultima8 {

DEFINE_RUNTIME_CLASSTYPE_CODE(QuickBarGump)

namespace {

const int CELL_W = 22;
const int CELL_H = 20;
const int TIP_H = 14;     // tooltip row above the icons
const int TIP_W = 150;    // the tooltip may be wider than the icons
const int MARGIN = 2;     // from the screen edge or the health bars
const int SHOW_ZONE = 6;  // the bar shows within this distance of it
const int TIP_FONT = 0;

enum IconArt { ART_MAIN, ART_GUMP, ART_MOUSE, ART_MAP, ART_ARROW };

enum Cheat { CHEAT_NONE, CHEAT_MAXSTATS, CHEAT_HEAL, CHEAT_INVINCIBLE, CHEAT_HACKMOVER };

struct Icon {
	KeybindingAction _action; //!< for the ACTIONS bar
	Cheat _cheat;             //!< for the CHEATS bar
	IconArt _art;
	int _shape, _frame;
	const char *_name; //!< English, translated in the "ui" context
	const char *_key;  //!< default key, or nullptr
};

// Pictures from the game: backpack 529, helmet 531, sword 243 (the
// weapon overlay), book 613, keyring 79, recall 833, bedroll 534
const Icon ACTION_ICONS[] = {
	{ ACTION_BACKPACK,  CHEAT_NONE, ART_MAIN, 529, 0,  "Backpack",   "I" },
	{ ACTION_INVENTORY, CHEAT_NONE, ART_MAIN, 531, 1,  "Inventory",  "Z" },
	{ ACTION_COMBAT,    CHEAT_NONE, ART_MAIN, 243, 68, "Combat",     "C" },
	{ ACTION_MINIMAP,   CHEAT_NONE, ART_MAP,  0,   0,  "Map",        "M" },
	{ ACTION_MENU,      CHEAT_NONE, ART_MAIN, 613, 10, "Menu",       "Esc" },
	{ ACTION_KEYRING,   CHEAT_NONE, ART_MAIN, 79,  0,  "Keyring",    "K" },
	{ ACTION_RECALL,    CHEAT_NONE, ART_MAIN, 833, 0,  "Recall",     "R" },
	{ ACTION_BEDROLL,   CHEAT_NONE, ART_MAIN, 534, 0,  "Bedroll",    "B" },
	{ ACTION_SAVE,      CHEAT_NONE, ART_MAIN, 613, 2,  "Save Game",  "F5" },
	{ ACTION_LOAD,      CHEAT_NONE, ART_MAIN, 613, 4,  "Load Game",  "F7" }
};

// red potion 766, round shield 539, the grabbing hand of the mouse cursors
const Icon CHEAT_ICONS[] = {
	{ ACTION_NONE, CHEAT_MAXSTATS,   ART_ARROW, 0,   0,  "Max Stats",     nullptr },
	{ ACTION_NONE, CHEAT_HEAL,       ART_MAIN,  766, 1,  "Heal",          nullptr },
	{ ACTION_NONE, CHEAT_INVINCIBLE, ART_MAIN,  539, 2,  "Invincibility", nullptr },
	{ ACTION_NONE, CHEAT_HACKMOVER,  ART_MOUSE, 0,   36, "Move Anything", nullptr }
};

const Icon *iconsOf(QuickBarGump::Kind kind) {
	return kind == QuickBarGump::CHEATS ? CHEAT_ICONS : ACTION_ICONS;
}

} // End of anonymous namespace

QuickBarGump::QuickBarGump(Kind kind)
	: Gump(0, 0, 0, 0, 0, FLAG_DONT_SAVE, LAYER_ABOVE_NORMAL), _kind(kind),
	  _shown(false), _hover(-1), _pressed(-1), _tooltipId(0) {
	if (kind == CHEATS) {
		_columns = ARRAYSIZE(CHEAT_ICONS);
		_rows = 1;
	} else {
		_columns = 5;
		_rows = 2;
	}
	int32 barW = _columns * CELL_W;
	int32 width = MAX<int32>(barW, TIP_W);
	// the icons in the corner, the tooltip extending away from it
	_cellsX = kind == CHEATS ? 0 : width - barW;
	_dims = Common::Rect32(0, 0, width, TIP_H + _rows * CELL_H);
}

QuickBarGump::~QuickBarGump() {
}

uint16 QuickBarGump::_barIds[2] = { 0, 0 };

QuickBarGump *QuickBarGump::find(Kind kind) {
	// the object id may have been given to another gump since
	QuickBarGump *bar = dynamic_cast<QuickBarGump *>(getGump(_barIds[kind]));
	if (bar && bar->_kind == kind && !bar->IsClosing())
		return bar;
	return nullptr;
}

void QuickBarGump::updateBars() {
	ConfMan.registerDefault("quick_bar", true);
	ConfMan.registerDefault("quick_bar_autohide", true);
	if (!GAME_IS_U8 || !Ultima8Engine::get_instance()->getDesktopGump())
		return;

	QuickBarGump *actions = find(ACTIONS);
	if (actions && !ConfMan.getBool("quick_bar"))
		actions->Close();
	else if (!actions && ConfMan.getBool("quick_bar"))
		(new QuickBarGump(ACTIONS))->InitGump(nullptr, false);

	// always there, but only used while cheats are enabled
	if (!find(CHEATS))
		(new QuickBarGump(CHEATS))->InitGump(nullptr, false);
}

void QuickBarGump::InitGump(Gump *newparent, bool take_focus) {
	Gump::InitGump(newparent, take_focus);
	_barIds[_kind] = getObjId();
	updatePosition();
}

bool QuickBarGump::isAvailable() const {
	return _kind == ACTIONS || Ultima8Engine::get_instance()->areCheatsEnabled();
}

void QuickBarGump::updatePosition() {
	Gump *desktop = Ultima8Engine::get_instance()->getDesktopGump();
	if (!desktop)
		return;
	Common::Rect32 screen = desktop->getDims();
	int32 barW = _columns * CELL_W;
	int32 barH = _rows * CELL_H;
	int32 bottom = screen.bottom - MARGIN;

	if (_kind == CHEATS) {
		Move(screen.left + MARGIN, bottom - barH - TIP_H);
		return;
	}

	// the bars of health and mana (opened from the paperdoll)
	int32 right = screen.right - MARGIN;
	Gump *stats = desktop->FindGump<MiniStatsGump>();
	if (stats && !stats->IsHidden() && !stats->IsClosing()) {
		Common::Rect32 sr = stats->getDims();
		stats->GumpRectToScreenSpace(sr);
		if (sr.right > screen.right - barW && sr.bottom > screen.bottom - barH - TIP_H) {
			right = sr.left - MARGIN;
			bottom = MIN<int32>(sr.bottom, bottom);
		}
	}

	Move(right - _dims.width(), bottom - barH - TIP_H);
}

void QuickBarGump::run() {
	Gump::run();
	updatePosition();

	// shown while the mouse is near
	int32 mx, my;
	Mouse::get_instance()->getMouseCoords(mx, my);
	ScreenSpaceToGump(mx, my);
	Common::Rect32 zone(_cellsX, TIP_H, _cellsX + _columns * CELL_W, TIP_H + _rows * CELL_H);
	zone.grow(SHOW_ZONE);
	bool shown = isAvailable() &&
	             (zone.contains(mx, my) || !ConfMan.getBool("quick_bar_autohide"));
	if (shown != _shown) {
		_shown = shown;
		if (!shown) {
			setHover(-1);
			_pressed = -1;
		}
	}
}

int QuickBarGump::iconAt(int32 x, int32 y) const {
	x -= _cellsX;
	if (x < 0 || x >= _columns * CELL_W || y < TIP_H || y >= TIP_H + _rows * CELL_H)
		return -1;
	return (y - TIP_H) / CELL_H * _columns + x / CELL_W;
}

bool QuickBarGump::PointOnGump(int mx, int my) {
	if (!_shown)
		return false;
	int32 gx = mx, gy = my;
	ParentToGump(gx, gy);
	return iconAt(gx, gy) >= 0;
}

uint16 QuickBarGump::TraceObjId(int32 mx, int32 my) {
	return PointOnGump(mx, my) ? getObjId() : 0;
}

Gump *QuickBarGump::onMouseDown(int button, int32 mx, int32 my) {
	if (button != Mouse::BUTTON_LEFT)
		return nullptr;
	int32 gx = mx, gy = my;
	ParentToGump(gx, gy);
	_pressed = iconAt(gx, gy);
	return _pressed >= 0 ? this : nullptr;
}

void QuickBarGump::onMouseUp(int button, int32 mx, int32 my) {
	if (button != Mouse::BUTTON_LEFT)
		return;
	int32 gx = mx, gy = my;
	ParentToGump(gx, gy);
	int index = iconAt(gx, gy);
	int pressed = _pressed;
	_pressed = -1;
	if (index >= 0 && index == pressed && isAvailable())
		doIcon(index);
}

void QuickBarGump::doIcon(int index) {
	const Icon &icon = iconsOf(_kind)[index];
	Ultima8Engine *engine = Ultima8Engine::get_instance();

	if (icon._cheat == CHEAT_NONE) {
		// the same as the key
		engine->handleActionDown(icon._action);
		engine->handleActionUp(icon._action);
		return;
	}

	// the same as the console commands (Cheat::...)
	MainActor *avatar = getMainActor();
	if (!avatar)
		return;
	switch (icon._cheat) {
	case CHEAT_MAXSTATS: {
		avatar->setStr(25);
		avatar->setDex(25);
		avatar->setInt(25);
		avatar->setHP(avatar->getMaxHP());
		avatar->setMana(avatar->getMaxMana());
		AudioProcess *audioproc = AudioProcess::get_instance();
		if (audioproc)
			audioproc->playSFX(0x36, 0x60, 1, 0);
		break;
	}
	case CHEAT_HEAL:
		avatar->setHP(avatar->getMaxHP());
		avatar->setMana(avatar->getMaxMana());
		break;
	case CHEAT_INVINCIBLE:
		if (avatar->hasActorFlags(Actor::ACT_INVINCIBLE))
			avatar->clearActorFlag(Actor::ACT_INVINCIBLE);
		else
			avatar->setActorFlag(Actor::ACT_INVINCIBLE);
		break;
	case CHEAT_HACKMOVER:
		engine->setHackMoverEnabled(!engine->isHackMoverEnabled());
		break;
	default:
		break;
	}
}

Gump *QuickBarGump::onMouseMotion(int32 mx, int32 my) {
	int32 gx = mx, gy = my;
	ParentToGump(gx, gy);
	setHover(_shown ? iconAt(gx, gy) : -1);
	return Gump::onMouseMotion(mx, my);
}

void QuickBarGump::onMouseLeft() {
	setHover(-1);
}

void QuickBarGump::setHover(int index) {
	if (index == _hover)
		return;
	_hover = index;

	Gump *old = getGump(_tooltipId);
	if (old)
		old->Close();
	_tooltipId = 0;
	if (index < 0)
		return;

	const Icon &icon = iconsOf(_kind)[index];
	Common::String text = Localization::uiText(icon._name, TIP_FONT);
	if (icon._key)
		text += Common::String::format(" (%s)", icon._key);
	TextWidget *tip = new TextWidget(0, 0, text, true, TIP_FONT, _dims.width(), 0,
	                                 _kind == CHEATS ? Font::TEXT_LEFT : Font::TEXT_RIGHT);
	tip->InitGump(this, false);
	// bottom of the text on the top of the icons
	Common::Rect32 d = tip->getDims();
	tip->Move(0, TIP_H - 1 - d.bottom);
	_tooltipId = tip->getObjId();
}

void QuickBarGump::PaintThis(RenderSurface *surf, int32 lerp_factor, bool scaled) {
	if (!_shown)
		return;

	for (int i = 0; i < _columns * _rows; i++) {
		int32 x = _cellsX + (i % _columns) * CELL_W;
		int32 y = TIP_H + (i / _columns) * CELL_H;
		Common::Rect32 cell(x, y, x + CELL_W, y + CELL_H);
		paintIcon(surf, i, cell);
	}
}

void QuickBarGump::paintIcon(RenderSurface *surf, int index, const Common::Rect32 &cell) {
	const Icon &icon = iconsOf(_kind)[index];
	MainActor *avatar = getMainActor();
	bool active = false;
	if (icon._action == ACTION_COMBAT)
		active = avatar && avatar->isInCombat();
	else if (icon._cheat == CHEAT_INVINCIBLE)
		active = avatar && avatar->hasActorFlags(Actor::ACT_INVINCIBLE);
	else if (icon._cheat == CHEAT_HACKMOVER)
		active = Ultima8Engine::get_instance()->isHackMoverEnabled();

	// a dark, see-through square with a frame; red while switched on
	Common::Rect32 inner(cell.left + 1, cell.top + 1, cell.right - 1, cell.bottom - 1);
	surf->fillBlended(active ? TEX32_PACK_RGBA(0x80, 0x10, 0x10, 0xA0)
	                         : TEX32_PACK_RGBA(0x10, 0x10, 0x18, 0xA0), inner);
	uint32 frame = index == _hover ? TEX32_PACK_RGB(0xF0, 0xD0, 0x60)
	                               : TEX32_PACK_RGB(0x60, 0x58, 0x48);
	surf->fill32(frame, inner.left, inner.top, inner.width(), 1);
	surf->fill32(frame, inner.left, inner.bottom - 1, inner.width(), 1);
	surf->fill32(frame, inner.left, inner.top, 1, inner.height());
	surf->fill32(frame, inner.right - 1, inner.top, 1, inner.height());

	if (icon._art == ART_MAP) {
		// a folded map: parchment, a road and a mark
		int32 x = cell.left + 5, y = cell.top + 5;
		surf->fill32(TEX32_PACK_RGB(0x6C, 0x50, 0x2C), x - 1, y - 1, 14, 12);
		surf->fill32(TEX32_PACK_RGB(0xD8, 0xC0, 0x88), x, y, 12, 10);
		surf->fill32(TEX32_PACK_RGB(0xB8, 0xA0, 0x68), x + 4, y, 1, 10);
		surf->fill32(TEX32_PACK_RGB(0xB8, 0xA0, 0x68), x + 8, y, 1, 10);
		surf->fill32(TEX32_PACK_RGB(0x50, 0x70, 0x30), x + 1, y + 6, 3, 2);
		surf->fill32(TEX32_PACK_RGB(0x40, 0x60, 0xA0), x + 9, y + 1, 2, 3);
		surf->fill32(TEX32_PACK_RGB(0xC0, 0x20, 0x20), x + 5, y + 4, 2, 2);
		return;
	}

	if (icon._art == ART_ARROW) {
		// a golden arrow pointing up, with a dark outline
		int32 cx = cell.left + CELL_W / 2, top = cell.top + 3;
		const uint32 dark = TEX32_PACK_RGB(0x40, 0x28, 0x08);
		const uint32 gold = TEX32_PACK_RGB(0xF0, 0xC0, 0x30);
		for (int row = 0; row < 7; row++) {
			surf->fill32(dark, cx - row - 1, top + row, 2 * row + 2, 1);
			if (row > 0)
				surf->fill32(gold, cx - row, top + row, 2 * row, 1);
		}
		surf->fill32(dark, cx - 4, top + 7, 8, 7);
		surf->fill32(gold, cx - 3, top + 7, 6, 6);
		return;
	}

	GameData *gamedata = GameData::get_instance();
	const Shape *shape;
	if (icon._art == ART_GUMP)
		shape = gamedata->getGumps()->getShape(icon._shape);
	else if (icon._art == ART_MOUSE)
		shape = gamedata->getMouse();
	else
		shape = gamedata->getMainShapes()->getShape(icon._shape);
	if (!shape || icon._frame >= (int)shape->frameCount())
		return;
	const ShapeFrame *f = shape->getFrame(icon._frame);
	if (!f)
		return;

	// centred in the cell; frames are drawn at their offset
	int32 x = cell.left + (CELL_W - f->_width) / 2 + f->_xoff;
	int32 y = cell.top + (CELL_H - f->_height) / 2 + f->_yoff;
	Common::Rect32 oldClip = surf->getClippingRect();
	Common::Rect32 clip(inner);
	clip.clip(oldClip);
	surf->setClippingRect(clip);
	surf->Paint(shape, icon._frame, x, y);
	surf->setClippingRect(oldClip);
}

} // End of namespace Ultima8
} // End of namespace Ultima
