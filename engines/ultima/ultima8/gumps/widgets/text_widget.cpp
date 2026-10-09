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

#include "ultima/ultima8/gumps/widgets/text_widget.h"

#include "ultima/ultima8/gfx/fonts/font_manager.h"
#include "ultima/ultima8/gfx/fonts/rendered_text.h"
#include "ultima/ultima8/gumps/ask_gump.h"
#include "ultima/ultima8/gumps/bark_gump.h"
#include "ultima/ultima8/gumps/widgets/button_widget.h"
#include "ultima/ultima8/ultima8.h"

namespace Ultima {
namespace Ultima8 {

DEFINE_RUNTIME_CLASSTYPE_CODE(TextWidget)

TextWidget::TextWidget() : Gump(), _gameFont(false), _fontNum(0), _blendColour(0),
		_tx(0), _ty(0), _currentStart(0), _currentEnd(0), _targetWidth(0), _targetHeight(0),
		_cachedText(nullptr), _textAlign(Font::TEXT_LEFT) {
}

TextWidget::TextWidget(int x, int y, const Common::String &txt, bool gamefont, int font,
					   int w, int h, Font::TextAlign align, bool dopaging) :
	Gump(x, y, w, h), _text(txt), _gameFont(gamefont), _fontNum(font),
	_blendColour(0), _currentStart(0), _currentEnd(0), _tx(0), _ty(0),
	_doPaging(dopaging), _targetWidth(w), _targetHeight(h),
	_cachedText(nullptr), _textAlign(align) {
}

TextWidget::~TextWidget(void) {
	delete _cachedText;
}

// Init the gump, call after construction
void TextWidget::InitGump(Gump *newparent, bool take_focus) {
	Gump::InitGump(newparent, take_focus);

	Font *font = getFont();

	// Y offset is always baseline
	_dims.moveTo(0, -textToGameY(font->getBaseline()));

	setupNextText();
}

int TextWidget::getVlead() {
	renderText();
	assert(_cachedText);

	return textToGameY(_cachedText->getVlead());
}

Font *TextWidget::getFont() const {
	if (_gameFont)
		return FontManager::get_instance()->getGameFont(_fontNum, true, true);
	else
		return FontManager::get_instance()->getTTFont(_fontNum);
}

// High-res fonts measure text in pixels of the high-res text layer (the
// window); the widget itself and its target size are in game pixels.
bool TextWidget::isHighResText() const {
	return _gameFont && getFont()->isHighRes();
}

int32 TextWidget::textToGameX(int32 v) const {
	return isHighResText() ? Ultima8Engine::get_instance()->hdToGameX(v) : v;
}

int32 TextWidget::textToGameY(int32 v) const {
	return isHighResText() ? Ultima8Engine::get_instance()->hdToGameY(v) : v;
}

int32 TextWidget::textTargetWidth() const {
	return isHighResText() ? Ultima8Engine::get_instance()->gameToHDX(_targetWidth) : _targetWidth;
}

int32 TextWidget::textTargetHeight() const {
	return isHighResText() ? Ultima8Engine::get_instance()->gameToHDY(_targetHeight) : _targetHeight;
}

void TextWidget::setTextDims(Font *font) {
	_dims.top = -textToGameY(font->getBaseline());
	_dims.left = 0;
	_dims.setWidth(textToGameX(_tx));
	_dims.setHeight(textToGameY(_ty));
}

bool TextWidget::setupNextText() {
	_currentStart = _currentEnd;

	if (_currentStart >= _text.size()) return false;

	Font *font = getFont();

	unsigned int remaining;
	font->getTextSize(_text.substr(_currentStart), _tx, _ty, remaining,
	                  textTargetWidth(), textTargetHeight(), _textAlign, true, _doPaging);

	setTextDims(font);
	_currentEnd = _currentStart + remaining;

	delete _cachedText;
	_cachedText = nullptr;

	return true;
}

void TextWidget::rewind() {
	_currentStart = 0;
	_currentEnd = 0;
	setupNextText();
}

void TextWidget::renderText() {
	if (!_cachedText) {
		Font *font = getFont();

		unsigned int remaining;
		_cachedText = font->renderText(_text.substr(_currentStart,
		                               _currentEnd - _currentStart),
		                               remaining, textTargetWidth(), textTargetHeight(),
		                               _textAlign, true, _doPaging);
	}
}

// Overloadable method to Paint just this Gump (RenderSurface is relative to this)
void TextWidget::PaintThis(RenderSurface *surf, int32 lerp_factor, bool scaled) {
	Gump::PaintThis(surf, lerp_factor, scaled);

	renderText();

	// drawn at the window resolution by PaintComposited
	if (scaled && isHighResText()) {
		return;
	}

	if (!_blendColour)
		_cachedText->draw(surf, 0, 0);
	else
		_cachedText->drawBlended(surf, 0, 0, _blendColour);
}

// Overloadable method to Paint just this gumps unscaled components that require compositing (RenderSurface is relative to parent).
void TextWidget::PaintComposited(RenderSurface *surf, int32 lerp_factor, int32 sx, int32 sy) {
	if (!isHighResText()) return;

	renderText();

	int32 x = 0, y = 0;
	GumpToScreenSpace(x, y, ROUND_BOTTOMRIGHT);
	x = ScaleCoord(x, sx);
	y = ScaleCoord(y, sy);

	if (!_blendColour)
		_cachedText->draw(surf, x, y);
	else
		_cachedText->drawBlended(surf, x, y, _blendColour);

	if (dynamic_cast<BarkGump *>(_parent))
		return;

	if (dynamic_cast<ButtonWidget *>(_parent) && dynamic_cast<AskGump *>(_parent->GetParent()))
		return;

	Common::Rect32 rect(_dims);
	GumpRectToScreenSpace(rect, ROUND_OUTSIDE);
}

// don't handle any mouse motion events, so let parent handle them for us.
Gump *TextWidget::onMouseMotion(int32 mx, int32 my) {
	return nullptr;
}


void TextWidget::saveData(Common::WriteStream *ws) {
	Gump::saveData(ws);

	ws->writeByte(_gameFont ? 1 : 0);
	ws->writeUint32LE(static_cast<uint32>(_fontNum));
	ws->writeUint32LE(_blendColour);
	// Offsets of the displayed text do not apply to the saved text
	bool saveOther = !_saveText.empty();
	const Common::String &text = saveOther ? _saveText : _text;
	ws->writeUint32LE(saveOther ? 0 : static_cast<uint32>(_currentStart));
	ws->writeUint32LE(saveOther ? 0 : static_cast<uint32>(_currentEnd));
	ws->writeUint32LE(static_cast<uint32>(_targetWidth));
	ws->writeUint32LE(static_cast<uint32>(_targetHeight));
	ws->writeUint16LE(static_cast<uint16>(_textAlign));
	ws->writeUint32LE(text.size());
	ws->write(text.c_str(), text.size());
}

bool TextWidget::loadData(Common::ReadStream *rs, uint32 version) {
	if (!Gump::loadData(rs, version)) return false;

	_gameFont = (rs->readByte() != 0);
	_fontNum = static_cast<int>(rs->readUint32LE());
	_blendColour = rs->readUint32LE();
	_currentStart = static_cast<int>(rs->readUint32LE());
	_currentEnd = static_cast<int>(rs->readUint32LE());
	_targetWidth = static_cast<int>(rs->readUint32LE());
	_targetHeight = static_cast<int>(rs->readUint32LE());
	_textAlign = static_cast<Font::TextAlign>(rs->readUint16LE());

	uint32 slen = rs->readUint32LE();
	if (slen > 0) {
		char *buf = new char[slen + 1];
		rs->read(buf, slen);
		buf[slen] = 0;
		_text = buf;
		delete[] buf;
	} else {
		_text = "";
	}

	// HACK ALERT: this is to deal with possibly changing font sizes
	// after loading.
	Font *font = getFont();

	unsigned int remaining;
	font->getTextSize(_text.substr(_currentStart), _tx, _ty, remaining,
	                  textTargetWidth(), textTargetHeight(), _textAlign, true, _doPaging);

	// Y offset is always baseline
	setTextDims(font);
	_currentEnd = _currentStart + remaining;

	return true;
}

} // End of namespace Ultima8
} // End of namespace Ultima
