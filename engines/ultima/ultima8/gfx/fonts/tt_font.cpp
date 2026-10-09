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

#include "ultima/ultima8/gfx/fonts/tt_font.h"

#include "ultima/ultima8/misc/common_types.h"
#include "ultima/ultima8/gfx/fonts/ttf_rendered_text.h"

// include iomanip

namespace Ultima {
namespace Ultima8 {

// various unicode characters which look like small black circles
static const uint16 BULLETS[] = { 0x2022, 0x30FB, 0x25CF, 0 };

TTFont::TTFont(Graphics::Font *font, uint32 rgb, int borderSize,
		bool antiAliased, bool SJIS, bool utf8) :
		_borderSize(borderSize), _ttfFont(font), _antiAliased(antiAliased), _SJIS(SJIS), _utf8(utf8),
		_letterSpacing(0), _cjkLatinSpacing(0), _lineSpacing(0),
		_PF_RGBA(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)) {
	_color = _PF_RGBA.RGBToColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xff, rgb & 0xff);

	_bullet = 0;
	// scan for a character to use as a conversation option _bullet
	for (int i = 0; BULLETS[i]; ++i) {
		Common::Rect box = font->getBoundingBox(BULLETS[i]);

		if (!box.isEmpty()) {
			_bullet = BULLETS[i];
			break;
		}
	}

	if (_bullet == 0) {
		_bullet = '*';
	}
}

TTFont::~TTFont() {

}

int TTFont::getHeight() {
	return _ttfFont->getFontHeight() + 2 * _borderSize; // constant (border)
}

int TTFont::getBaseline() {
	Common::Rect box = _ttfFont->getBoundingBox('W');
	return box.bottom;
}

int TTFont::getBaselineSkip() {
	// TODO: Come up with something more generic than just hardcoding 2 pixel line separation
	return getHeight() + 2 + _lineSpacing;
}

bool TTFont::isCJK(uint32 c) {
	return (c >= 0x3040 && c <= 0x30FF) ||    // hiragana, katakana
	       (c >= 0x3400 && c <= 0x4DBF) ||    // CJK extension A
	       (c >= 0x4E00 && c <= 0x9FFF) ||    // CJK unified ideographs
	       (c >= 0xAC00 && c <= 0xD7AF) ||    // hangul
	       (c >= 0xF900 && c <= 0xFAFF) ||    // CJK compatibility ideographs
	       (c >= 0x20000 && c <= 0x3FFFF);    // CJK extensions B and later
}

bool TTFont::isLatinAlnum(uint32 c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	       (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7);  // Latin-1 and Extended-A/B letters
}

int TTFont::spacingBetween(uint32 a, uint32 b) const {
	int extra = _letterSpacing;
	if ((isCJK(a) && isLatinAlnum(b)) || (isLatinAlnum(a) && isCJK(b)))
		extra += _cjkLatinSpacing;
	return extra;
}

int TTFont::textWidth(const Common::U32String &text) const {
	if (!hasSpacing())
		return _ttfFont->getStringWidth(text);

	// the same advances as Graphics::Font::getStringWidth, plus the spacing
	int width = 0;
	uint32 last = 0;
	for (uint i = 0; i < text.size(); ++i) {
		uint32 c = text[i];
		width += _ttfFont->getKerningOffset(last, c);
		if (i > 0)
			width += spacingBetween(last, c);
		width += _ttfFont->getCharWidth(c);
		last = c;
	}
	return width;
}

void TTFont::drawSpaced(Graphics::ManagedSurface &surf, const Common::U32String &text, uint32 color) const {
	int x = 0;
	uint32 last = 0;
	for (uint i = 0; i < text.size(); ++i) {
		uint32 c = text[i];
		x += _ttfFont->getKerningOffset(last, c);
		if (i > 0)
			x += spacingBetween(last, c);
		if (x >= surf.w)
			break;
		_ttfFont->drawChar(&surf, c, x, 0, color);
		x += _ttfFont->getCharWidth(c);
		last = c;
	}
}

template<class T>
static Common::U32String toUnicode(const Common::String &text, uint16 bullet) {
	// Build by appending. Pre-sizing with the (UTF-8 decoding)
	// U32String(const char *, len) constructor yields fewer than len
	// characters for multi-byte text, and setChar() then asserts.
	Common::String::size_type len = T::length(text);
	Common::U32String result;

	Common::String::const_iterator iter = text.begin();
	for (uint idx = 0; idx < len; ++idx) {
		uint32 u = T::unicode(iter);
		if (u == '@') {
			result += (Common::u32char_type_t)bullet;
		} else {
			result += (Common::u32char_type_t)u;
		}
	}

	return result;
}

void TTFont::getStringSize(const Common::String &text, int32 &width, int32 &height) {
	// convert to unicode
	Common::U32String unicodeText;
	if (_utf8)
		unicodeText = toUnicode<UTF8Traits>(text, _bullet);
	else if (!_SJIS)
		unicodeText = toUnicode<Traits>(text, _bullet);
	else
		unicodeText = toUnicode<SJISTraits>(text, _bullet);

	width = textWidth(unicodeText);
	height = _ttfFont->getFontHeight();

	width += 2 * _borderSize;
	height += 2 * _borderSize;
}

void TTFont::getTextSize(const Common::String &text,
						 int32 &resultWidth, int32 &resultHeight,
						 unsigned int &remaining,
						 int32 width, int32 height, TextAlign align,
						 bool u8specials, bool pagebreaks) {
	Common::List<PositionedText> tmp;
	if (_utf8)
		tmp = typesetText<UTF8Traits>(this, text, remaining,
		                              width, height, align, u8specials, pagebreaks,
		                              resultWidth, resultHeight);
	else if (!_SJIS)
		tmp = typesetText<Traits>(this, text, remaining,
		                          width, height, align, u8specials, pagebreaks,
		                          resultWidth, resultHeight);
	else
		tmp = typesetText<SJISTraits>(this, text, remaining,
		                              width, height, align, u8specials, pagebreaks,
		                              resultWidth, resultHeight);
}


void TTFont::addTextBorder(Graphics::ManagedSurface &textSurf, uint32 *texBuf, const Common::Rect32 &dims, int32 resultWidth, int32 resultHeight, uint32 borderColor) {
	uint8 bA, bR, bG, bB;
	_PF_RGBA.colorToARGB(borderColor, bA, bR, bG, bB);

	int sqrSize = _borderSize * _borderSize;
	int sqrEdge = (_borderSize + 1) * (_borderSize + 1);

	for (int y = 0; y < textSurf.h; y++) {
		const byte* surfrow = (const byte*)textSurf.getBasePtr(0, y);

		for (int x = 0; x < textSurf.w; x++) {
			if (_antiAliased) {
				uint32 sColor = *((const uint32 *)(surfrow + x * 4));
				uint8 sR, sG, sB, sA;
				_PF_RGBA.colorToARGB(sColor, sA, sR, sG, sB);

				if (sA == 0x00)
					continue;

				for (int dx = -_borderSize; dx <= _borderSize; dx++) {
					for (int dy = -_borderSize; dy <= _borderSize; dy++) {
						int tx = dims.left + x + _borderSize + dx;
						int ty = dims.top + y + _borderSize + dy;
						if (tx >= 0 && tx < resultWidth && ty >= 0 && ty < resultHeight) {
							uint32 dColor = texBuf[ty * resultWidth + tx];
							if (borderColor != dColor) {
								int sqrDist = (dx * dx) + (dy * dy);
								if (sqrDist < sqrSize) {
									texBuf[ty * resultWidth + tx] = borderColor;
								}
								else if (sqrDist < sqrEdge) {
									// Blend border color at source intensity with destination
									uint8 dA, dR, dG, dB;
									_PF_RGBA.colorToARGB(dColor, dA, dR, dG, dB);

									double bAlpha = (double)bA / 255.0;
									double sAlpha = (double)sA / 255.0;
									double dAlpha = (double)dA / 255.0;
									dAlpha *= (1.0 - sAlpha);

									dR = static_cast<uint8>((bR * sAlpha + dR * dAlpha) / (sAlpha + dAlpha));
									dG = static_cast<uint8>((bG * sAlpha + dG * dAlpha) / (sAlpha + dAlpha));
									dB = static_cast<uint8>((bB * sAlpha + dB * dAlpha) / (sAlpha + dAlpha));
									dA = static_cast<uint8>(255. * bAlpha * (sAlpha + dAlpha));

									texBuf[ty * resultWidth + tx] = _PF_RGBA.ARGBToColor(dA, dR, dG, dB);
								}
							}
						}
					}
				}
			}
			else if (surfrow[x] == 1) {
				for (int dx = -_borderSize; dx <= _borderSize; dx++) {
					for (int dy = -_borderSize; dy <= _borderSize; dy++) {
						int tx = dims.left + x + _borderSize + dx;
						int ty = dims.top + y + _borderSize + dy;
						if (tx >= 0 && tx < resultWidth && ty >= 0 && ty < resultHeight) {
							int sqrDist = (dx * dx) + (dy * dy);
							if (sqrDist < sqrEdge) {
								texBuf[ty * resultWidth + tx] = borderColor;
							}
						}
					}
				}
			}
		}
	}
}

RenderedText *TTFont::renderText(const Common::String &text, unsigned int &remaining,
		int32 width, int32 height, TextAlign align, bool u8specials, bool pagebreaks,
		Common::String::size_type cursor) {
	int32 resultWidth, resultHeight, lineHeight;
	Common::List<PositionedText> lines;
	if (_utf8)
		lines = typesetText<UTF8Traits>(this, text, remaining, width, height, align, u8specials, pagebreaks,
			resultWidth, resultHeight, cursor);
	else if (!_SJIS)
		lines = typesetText<Traits>(this, text, remaining, width, height, align, u8specials, pagebreaks,
			resultWidth, resultHeight, cursor);
	else
		lines = typesetText<SJISTraits>(this, text, remaining, width, height, align, u8specials, pagebreaks,
			resultWidth, resultHeight, cursor);
	lineHeight = _ttfFont->getFontHeight();

	uint32 borderColor = _PF_RGBA.ARGBToColor(0xFF, 0x00, 0x00, 0x00);

	Graphics::ManagedSurface *texture = new Graphics::ManagedSurface(resultWidth, resultHeight, _PF_RGBA);
	uint32 *texBuf = (uint32 *)texture->getPixels();

	for (const auto &line : lines) {
		// convert to unicode
		Common::U32String unicodeText;
		if (_utf8)
			unicodeText = toUnicode<UTF8Traits>(line._text, _bullet);
		else if (!_SJIS)
			unicodeText = toUnicode<Traits>(line._text, _bullet);
		else
			unicodeText = toUnicode<SJISTraits>(line._text, _bullet);

		// Create a surface and render the text
		Graphics::ManagedSurface textSurf;

		if (!_antiAliased) {
			// When not in antialiased mode, use a paletted surface where '1' is
			// used for pixels of the text
			textSurf.create(resultWidth, lineHeight, Graphics::PixelFormat::createFormatCLUT8());
			if (hasSpacing())
				drawSpaced(textSurf, unicodeText, 1);
			else
				_ttfFont->drawString(&textSurf, unicodeText, 0, 0, resultWidth, 1);
		} else {
			// Use a high color surface with the specified _color color for text
			textSurf.create(resultWidth, lineHeight, _PF_RGBA);
			if (hasSpacing())
				drawSpaced(textSurf, unicodeText, _color);
			else
				_ttfFont->drawString(&textSurf, unicodeText, 0, 0, resultWidth, _color);
		};

		// Add border within radius. Pixels on the edge are alpha blended if antialiased
		if (_borderSize > 0) {
			addTextBorder(textSurf, texBuf, line._dims, resultWidth, resultHeight, borderColor);
		}

		// render the text surface into our texture buffer
		for (int y = 0; y < textSurf.h; y++) {
			const byte *surfrow = (const byte *)textSurf.getBasePtr(0, y);

			int ty = line._dims.top + y + _borderSize;
			if (ty < 0 || ty >= resultHeight)
				continue;
			for (int x = 0; x < textSurf.w; x++) {
				int tx = line._dims.left + x + _borderSize;
				if (tx < 0 || tx >= resultWidth)
					continue;
				if (_antiAliased) {
					uint32 sColor = *((const uint32 *)(surfrow + x * 4));
					uint8 sR, sG, sB, sA;
					_PF_RGBA.colorToARGB(sColor, sA, sR, sG, sB);

					if (sA == 0xFF) {
						texBuf[ty * resultWidth + tx] = sColor;
					}
					else if (sA != 0x00) {
						// Blend color with destination
						int32 dColor = texBuf[ty * resultWidth + tx];
						uint8 dA, dR, dG, dB;
						_PF_RGBA.colorToARGB(dColor, dA, dR, dG, dB);

						double sAlpha = (double)sA / 255.0;
						double dAlpha = (double)dA / 255.0;
						dAlpha *= (1.0 - sAlpha);

						dR = static_cast<uint8>((sR * sAlpha + dR * dAlpha) / (sAlpha + dAlpha));
						dG = static_cast<uint8>((sG * sAlpha + dG * dAlpha) / (sAlpha + dAlpha));
						dB = static_cast<uint8>((sB * sAlpha + dB * dAlpha) / (sAlpha + dAlpha));
						dA = static_cast<uint8>(255. * (sAlpha + dAlpha));

						texBuf[ty * resultWidth + tx] = _PF_RGBA.ARGBToColor(dA, dR, dG, dB);
					}
				}
				else if (surfrow[x] == 1) {
					texBuf[ty * resultWidth + tx] = _color;
				}
			}
		}

		if (line._cursor != Common::String::npos) {
			assert(line._cursor <= line._text.size());
			// The cursor is a byte offset; convert the text before it the
			// same way as the line (a multi-byte character is one glyph)
			Common::String before = line._text.substr(0, line._cursor);
			Common::U32String unicodeBefore;
			if (_utf8)
				unicodeBefore = toUnicode<UTF8Traits>(before, _bullet);
			else if (!_SJIS)
				unicodeBefore = toUnicode<Traits>(before, _bullet);
			else
				unicodeBefore = toUnicode<SJISTraits>(before, _bullet);

			int w = textWidth(unicodeBefore);

			// The cursor can be just past the end of the line or below a
			// box lower than the font: stay inside the texture
			int tx = line._dims.left + w + _borderSize;
			if (tx >= 0 && tx < resultWidth) {
				for (int y = 0; y < line._dims.height(); y++) {
					int ty = line._dims.top + y;
					if (ty >= 0 && ty < resultHeight)
						texBuf[ty * resultWidth + tx] = borderColor;
				}
			}
		}
	}

	return new TTFRenderedText(texture, resultWidth, resultHeight,
		getBaselineSkip() - getHeight(), getBaseline(), isAntialiased());
}

} // End of namespace Ultima8
} // End of namespace Ultima
