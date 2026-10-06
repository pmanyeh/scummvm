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

#ifndef ULTIMA8_GUMPS_ASKGUMP_H
#define ULTIMA8_GUMPS_ASKGUMP_H

#include "common/array.h"
#include "common/str.h"
#include "ultima/ultima8/gumps/item_relative_gump.h"
#include "ultima/ultima8/misc/classtype.h"

namespace Ultima {
namespace Ultima8 {

class UCList;

/**
 * Represents asking a question while talking to an NPC
 */
class AskGump : public ItemRelativeGump {
	UCList *_answers;
	//! localized texts to show instead of the answers (not saved)
	Common::Array<Common::String> _displayAnswers;
public:
	// p_dynamic_class stuff
	ENABLE_RUNTIME_CLASSTYPE()

	AskGump();
	AskGump(uint16 owner, UCList *answers,
	        const Common::Array<Common::String> &displayAnswers = Common::Array<Common::String>());
	~AskGump() override;

	// Init the gump, call after construction
	void        InitGump(Gump *newparent, bool take_focus = true) override;

	void        ChildNotify(Gump *child, uint32 message) override;

	bool loadData(Common::ReadStream *rs, uint32 version);
	void saveData(Common::WriteStream *ws) override;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
