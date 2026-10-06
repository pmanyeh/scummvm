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

#ifndef ULTIMA8_GFX_FONTS_FONT_H
#define ULTIMA8_GFX_FONTS_FONT_H

#include "common/list.h"
#include "common/rect.h"
#include "common/str.h"
#include "ultima/ultima8/misc/encoding.h"

namespace Ultima {
namespace Ultima8 {

class RenderedText;

struct PositionedText {
	Common::String _text;
	Common::Rect32 _dims;
	Common::String::size_type _cursor;
};

class Font {
public:
	Font();
	virtual ~Font();

	enum TextAlign {
		TEXT_LEFT,
		TEXT_CENTER,
		TEXT_RIGHT
	};

	//! get the height of the font
	virtual int getHeight() = 0;

	//! get the baseline of the font (relative from the top)
	virtual int getBaseline() = 0;

	//! get the baselineskip of the font (distance between two baselines)
	virtual int getBaselineSkip() = 0;

	//! get the dimensions of a string (not containing any newlines)
	//! \param text The string
	//! \param width Returns the width
	//! \param height Returns the height
	virtual void getStringSize(const Common::String &text,
		int32 &width, int32 &height) = 0;

	//! render a string
	//! \param text The text
	//! \param remaining Returns index of the first character not printed
	//! \param width The width of the target rectangle, or 0 for unlimited
	//! \param height The height of the target rectangle, or 0 for unlimited
	//! \param align Alignment of the text (left, right, center)
	//! \param u8specials If true, interpret the special characters U8 uses
	//! \param pagebreaks If true (and u8specials too), stop at U8 pagebreaks
	//! \return the rendered text in a RenderedText object
	virtual RenderedText *renderText(const Common::String &text,
	    unsigned int &remaining, int32 width = 0, int32 height = 0,
		TextAlign align = TEXT_LEFT, bool u8specials = false,
		bool pagebreaks = false,
		Common::String::size_type cursor = Common::String::npos) = 0;

	//! get the dimensions of a rendered string
	//! \param text The text
	//! \param resultwidth Returns the resulting width
	//! \param resultheight Returns the resulting height
	//! \param remaining Returns index of the first character not printed
	//! \param width The width of the target rectangle, or 0 for unlimited
	//! \param height The height of the target rectangle, or 0 for unlimited
	//! \param align Alignment of the text (left, right, center)
	//! \param u8specials If true, interpret the special characters U8 uses
	//! \param pagebreaks If true (and u8specials too), stop at U8 pagebreaks
	virtual void getTextSize(const Common::String &text,
		int32 &resultwidth, int32 &resultheight, unsigned int &remaining,
		int32 width = 0, int32 height = 0, TextAlign align = TEXT_LEFT,
		bool u8specials = false, bool pagebreaks = false);

	void setHighRes(bool hr) {
		_highRes = hr;
	}

	//! true if the font renders UTF-8 text (localized text needs this)
	virtual bool isUTF8() const {
		return false;
	}
	bool isHighRes() const {
		return _highRes;
	}

protected:
	bool _highRes;

	struct Traits {
		static bool isSpace(Common::String::const_iterator &i, bool u8specials) {
			char c = *i;
			return (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
			        (u8specials && (c == '%' || c == '~' || c == '*' || c == '^')));
		}
		static bool isTab(Common::String::const_iterator &i, bool u8specials) {
			char c = *i;
			return (c == '\t' ||
			        (u8specials && (c == '\t' || c == '%')));
		}
		static bool isBreak(Common::String::const_iterator &i, bool u8specials) {
			char c = *i;
			return (c == '\n' ||
			        (u8specials && (c == '\n' || c == '~' || c == '*')));
		}
		static bool isPageBreak(Common::String::const_iterator &i, bool u8specials) {
			char c = *i;
			return (u8specials && c == '*');
		}
		static bool canBreakAfter(Common::String::const_iterator &i);
		// True if a line may break after this character even without a
		// following space (CJK). Always false for 8-bit and SJIS text.
		static bool endsWord(Common::String::const_iterator &i) {
			return false;
		}
		static void advance(Common::String::const_iterator &i) {
			++i;
		}
		static Common::String::size_type length(const Common::String &t) {
			return t.size();
		}
		static uint32 unicode(Common::String::const_iterator &i) {
			return encoding[static_cast<uint8>(*i++)];
		}
	};
	struct SJISTraits : public Traits {
		static bool canBreakAfter(Common::String::const_iterator &i);
		static void advance(Common::String::const_iterator &i) {
			// FIXME: this can advance past the end of a malformed string
			uint8 c = *i;
			i++;
			if (c >= 0x80) i++;
		}
		static Common::String::size_type length(const Common::String &t) {
			Common::String::size_type l = 0;
			Common::String::const_iterator iter = t.begin();
			while (iter != t.end()) {
				advance(iter);
				l++;
			}
			return l;
		}
		static uint32 unicode(Common::String::const_iterator &i) {
			uint16 s = static_cast<uint8>(*i);
			i++;
			if (s >= 0x80) {
				uint16 t = static_cast<uint8>(*i++);
				s |= (t << 8);
			}
			return shiftjis_to_unicode(s);
		}
	};
	// UTF-8 text (localized display strings).
	// A byte that does not start a valid UTF-8 sequence is decoded on its own
	// with the legacy 8-bit table, so original game text (which contains a few
	// CP437 accented characters) still renders exactly as before.
	// Common::String is NUL terminated, so validation never reads past the end.
	struct UTF8Traits : public Traits {
		static bool canBreakAfter(Common::String::const_iterator &i);
		static bool endsWord(Common::String::const_iterator &i);
		//! Length of the valid UTF-8 sequence at i, or 0 if it is not valid
		static int sequenceLength(Common::String::const_iterator i) {
			uint8 c = *i;
			int n;
			if (c < 0x80)
				return 1;
			else if (c >= 0xC2 && c <= 0xDF)
				n = 2;
			else if (c >= 0xE0 && c <= 0xEF)
				n = 3;
			else if (c >= 0xF0 && c <= 0xF4)
				n = 4;
			else
				return 0;
			for (int k = 1; k < n; k++) {
				if ((static_cast<uint8>(i[k]) & 0xC0) != 0x80)
					return 0;
			}
			return n;
		}
		static void advance(Common::String::const_iterator &i) {
			int n = sequenceLength(i);
			i += n ? n : 1;
		}
		static Common::String::size_type length(const Common::String &t) {
			Common::String::size_type l = 0;
			Common::String::const_iterator iter = t.begin();
			while (iter != t.end()) {
				advance(iter);
				l++;
			}
			return l;
		}
		static uint32 unicode(Common::String::const_iterator &i) {
			int n = sequenceLength(i);
			uint8 c = *i++;
			if (n == 0)
				return encoding[c];
			if (n == 1)
				return c;
			uint32 u = c & (0xFF >> (n + 1));
			for (int k = 1; k < n; k++)
				u = (u << 6) | (static_cast<uint8>(*i++) & 0x3F);
			return u;
		}
	};
};

template<class T>
Common::List<PositionedText> typesetText(Font *font,
	const Common::String &text, unsigned int &remaining,
	int32 width, int32 height, Font::TextAlign align,
	bool u8specials, bool pagebreaks,
	int32 &resultwidth, int32 &resultheight,
	Common::String::size_type cursor = Common::String::npos);

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
