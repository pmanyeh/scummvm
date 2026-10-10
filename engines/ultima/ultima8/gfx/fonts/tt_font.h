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

#ifndef ULTIMA8_GFX_FONTS_TTFONT_H
#define ULTIMA8_GFX_FONTS_TTFONT_H

#include "ultima/ultima8/gfx/fonts/font.h"
#include "graphics/font.h"
#include "graphics/pixelformat.h"

namespace Ultima {
namespace Ultima8 {

class TTFont : public Font {
public:
	TTFont(Graphics::Font *font, uint32 rgb, int bordersize,
	       bool antiAliased, bool SJIS, bool utf8 = false);
	~TTFont() override;

	int getHeight() override;
	int getBaseline() override;
	int getBaselineSkip() override;

	bool isAntialiased() {
		return _antiAliased;
	}

	bool isUTF8() const override {
		return _utf8;
	}

	/**
	 * Extra spacing in pixels (all 0 by default):
	 * \param letter   between any two characters
	 * \param cjkLatin between a CJK character and a Latin letter or digit
	 * \param line     added to the 2 pixels between lines
	 */
	void setSpacing(int letter, int cjkLatin, int line) {
		_letterSpacing = letter;
		_cjkLatinSpacing = cjkLatin;
		_lineSpacing = line;
	}

	/**
	 * Draw a copy of the text dx, dy pixels away in another colour under it,
	 * instead of the border (like a carved or embossed letter). The border
	 * size becomes the room around the text for it.
	 */
	void setShadow(uint32 rgb, int dx, int dy);

	//! Width of a line of text, with the extra spacing
	int textWidth(const Common::U32String &text) const;

	//! CJK ideograph, kana or hangul (not punctuation)
	static bool isCJK(uint32 c);
	//! Latin letter or digit
	static bool isLatinAlnum(uint32 c);

	void getStringSize(const Common::String &text,
		int32 &width, int32 &height) override;

	void getTextSize(const Common::String &text,
		int32 &resultwidth, int32 &resultheight, unsigned int &remaining,
		int32 width = 0, int32 height = 0, TextAlign align = TEXT_LEFT,
		bool u8specials = false, bool pagebreaks = false) override;

	RenderedText *renderText(const Common::String &text,
		unsigned int &remaining, int32 width = 0, int32 height = 0,
		TextAlign align = TEXT_LEFT, bool u8specials = false,
		bool pagebreaks = false,
		Common::String::size_type cursor = Common::String::npos) override;

protected:
	Graphics::Font *_ttfFont;
	uint32 _color;
	int _borderSize;
	bool _antiAliased;
	bool _SJIS;
	bool _utf8;
	Graphics::PixelFormat _PF_RGBA;

	uint16 _bullet;

	int _letterSpacing;
	int _cjkLatinSpacing;
	int _lineSpacing;

	bool _shadow;
	uint32 _shadowColor;
	int _shadowDx, _shadowDy;

	bool hasSpacing() const {
		return _letterSpacing != 0 || _cjkLatinSpacing != 0;
	}
	//! Extra pixels between characters a and b
	int spacingBetween(uint32 a, uint32 b) const;
	//! Draw a line character by character with the extra spacing
	void drawSpaced(Graphics::ManagedSurface &surf, const Common::U32String &text, uint32 color) const;

	void addTextBorder(Graphics::ManagedSurface &textSurf, uint32 *texBuf, const Common::Rect32 &dims, int32 resultWidth, int32 resultHeight, uint32 borderColor);
	//! Blend a rendered line into the texture at (x, y); in color if not 0
	void blendLine(const Graphics::ManagedSurface &textSurf, uint32 *texBuf, int32 x, int32 y,
	               int32 resultWidth, int32 resultHeight, uint32 color) const;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
