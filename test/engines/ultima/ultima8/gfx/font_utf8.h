#include <cxxtest/TestSuite.h>
#include "engines/ultima/ultima8/gfx/fonts/font.h"

// Strings are UTF-8 written as escapes. Adjacent literals split escapes from
// following hex digits ("\x9A" "100").
#define ZH_FANTI_TEST "\xE7\xB9\x81\xE9\xAB\x94\xE4\xB8\xAD\xE6\x96\x87\xE6\xB8\xAC\xE8\xA9\xA6"     // 繁體中文測試
#define ZH_PAGAN      "Ultima VIII\xEF\xBC\x9A\xE7\x95\xB0\xE6\x95\x99\xE5\xBE\x92"                    // Ultima VIII：異教徒
#define ZH_HELLO      "\xE4\xBD\xA0\xE5\xA5\xBD\xEF\xBC\x8C" "Avatar\xEF\xBC\x81"                       // 你好，Avatar！
#define ZH_HP         "\xE7\x94\x9F\xE5\x91\xBD\xE5\x80\xBC HP\xEF\xBC\x9A" "100"                       // 生命值 HP：100
#define ZH_QUOTE      "\xE3\x80\x8C\xE9\x80\x99\xE6\x98\xAF\xE4\xB8\x80\xE6\xAE\xB5\xE4\xB8\xAD\xE6\x96\x87\xE3\x80\x82\xE3\x80\x8D" // 「這是一段中文。」
#define ZH_COMMA      "\xEF\xBC\x8C"                                                                     // ，

namespace {

/**
 * Fixed metrics font: every character (codepoint) is 10 pixels wide and
 * 10 high; lines are 12 apart.
 */
class TestFont : public Ultima::Ultima8::Font {
public:
	typedef Font::Traits LegacyTraits;
	typedef Font::SJISTraits SJIS;
	typedef Font::UTF8Traits UTF8;

	bool _utf8;
	TestFont(bool utf8 = true) : _utf8(utf8) {}

	int getHeight() override { return 10; }
	int getBaseline() override { return 8; }
	int getBaselineSkip() override { return 12; }
	void getStringSize(const Common::String &text, int32 &width, int32 &height) override {
		width = 10 * (_utf8 ? UTF8::length(text) : LegacyTraits::length(text));
		height = 10;
	}
	Ultima::Ultima8::RenderedText *renderText(const Common::String &, unsigned int &,
			int32, int32, TextAlign, bool, bool, Common::String::size_type) override {
		return nullptr;
	}
};

template<class T>
Common::Array<Common::String> typeset(TestFont &font, const Common::String &text,
		int32 width, int32 height, unsigned int &remaining, bool pagebreaks = false) {
	int32 rw, rh;
	Common::List<Ultima::Ultima8::PositionedText> lines =
		Ultima::Ultima8::typesetText<T>(&font, text, remaining, width, height,
		                                Ultima::Ultima8::Font::TEXT_LEFT, true, pagebreaks, rw, rh);
	Common::Array<Common::String> out;
	for (const auto &l : lines)
		out.push_back(l._text);
	return out;
}

// Decode a whole string; also checks that unicode() and advance()/length() agree
Common::Array<uint32> decode(const Common::String &s) {
	Common::Array<uint32> out;
	Common::String::const_iterator i = s.begin();
	while (i != s.end())
		out.push_back(TestFont::UTF8::unicode(i));
	return out;
}

} // End of anonymous namespace

/**
 * Test suite for UTF-8 / CJK text in engines/ultima/ultima8/gfx/fonts/font.h
 */
class U8FontUTF8TestSuite : public CxxTest::TestSuite {
public:
	void test_decode_required_strings() {
		const char *strings[] = { ZH_FANTI_TEST, ZH_PAGAN, ZH_HELLO, ZH_HP, ZH_QUOTE };
		const uint lengths[] = { 6, 15, 10, 10, 9 };
		for (int n = 0; n < 5; n++) {
			Common::String s(strings[n]);
			TS_ASSERT_EQUALS(TestFont::UTF8::length(s), lengths[n]);
			TS_ASSERT_EQUALS(decode(s).size(), lengths[n]);
		}

		Common::Array<uint32> u = decode(ZH_FANTI_TEST);
		TS_ASSERT_EQUALS(u[0], 0x7E41u); // 繁
		TS_ASSERT_EQUALS(u[1], 0x9AD4u); // 體
		u = decode(ZH_HP);
		TS_ASSERT_EQUALS(u[3], (uint32)' ');
		TS_ASSERT_EQUALS(u[6], 0xFF1Au); // ：
		TS_ASSERT_EQUALS(u[9], (uint32)'0');
	}

	void test_decode_malformed() {
		using Ultima::Ultima8::encoding;

		// truncated sequence: each byte falls back to the legacy table
		Common::String s1("\xE7\xB9");
		TS_ASSERT_EQUALS(decode(s1).size(), 2u);
		TS_ASSERT_EQUALS(decode(s1)[0], (uint32)encoding[0xE7]);
		TS_ASSERT_EQUALS(decode(s1)[1], (uint32)encoding[0xB9]);
		TS_ASSERT_EQUALS(TestFont::UTF8::length(s1), 2u);

		// stray continuation byte
		Common::String s2("\x80" "A");
		TS_ASSERT_EQUALS(decode(s2).size(), 2u);
		TS_ASSERT_EQUALS(decode(s2)[0], (uint32)encoding[0x80]);
		TS_ASSERT_EQUALS(decode(s2)[1], (uint32)'A');

		// invalid lead byte followed by continuation bytes
		Common::String s3("\xFF\x80\x80" "B");
		TS_ASSERT_EQUALS(decode(s3).size(), 4u);
		TS_ASSERT_EQUALS(decode(s3)[3], (uint32)'B');
		TS_ASSERT_EQUALS(TestFont::UTF8::length(s3), 4u);

		// valid sequence followed by a surplus continuation byte
		Common::String s4("\xC3\xA9\x80" "X");
		TS_ASSERT_EQUALS(decode(s4).size(), 3u);
		TS_ASSERT_EQUALS(decode(s4)[0], 0xE9u);
		TS_ASSERT_EQUALS(decode(s4)[1], (uint32)encoding[0x80]);

		// lead byte followed by ASCII: the ASCII characters survive
		Common::String s5("\xE4" "AB");
		TS_ASSERT_EQUALS(decode(s5).size(), 3u);
		TS_ASSERT_EQUALS(decode(s5)[1], (uint32)'A');
	}

	void test_original_game_text_unchanged() {
		// The English game contains a few CP437 characters (German/French
		// phrases). In UTF-8 mode they must decode exactly like the legacy path.
		const char *strings[] = {
			"Ich hei\xE1" "e ",
			"Enchant\x82, ",
			"Ah, wirklich ein sch\x94ner Name",
			"Je pr\x82" "f\x8Are ne point le dire."
		};
		for (int n = 0; n < 4; n++) {
			Common::String s(strings[n]);
			TS_ASSERT_EQUALS(TestFont::UTF8::length(s), TestFont::LegacyTraits::length(s));
			Common::String::const_iterator a = s.begin(), b = s.begin();
			while (a != s.end()) {
				TS_ASSERT_EQUALS(TestFont::UTF8::unicode(a), TestFont::LegacyTraits::unicode(b));
			}
		}
	}

	void test_kinsoku() {
		Common::String s("\xE4\xB8\xAD" ZH_COMMA "\xE6\x96\x87"); // 中，文
		Common::String::const_iterator i = s.begin();
		TS_ASSERT(!TestFont::UTF8::canBreakAfter(i));              // 中|，
		TestFont::UTF8::advance(i);
		TS_ASSERT(TestFont::UTF8::canBreakAfter(i));               // ，|文

		Common::String open("\xE3\x80\x8C\xE9\x80\x99");            // 「這
		i = open.begin();
		TS_ASSERT(!TestFont::UTF8::canBreakAfter(i));

		Common::String latin("ab");
		i = latin.begin();
		TS_ASSERT(!TestFont::UTF8::canBreakAfter(i));
		TS_ASSERT(!TestFont::UTF8::endsWord(i));
	}

	void test_wrap_pure_chinese() {
		TestFont font;
		unsigned int remaining;
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font, ZH_FANTI_TEST, 40, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 2u);
		TS_ASSERT_EQUALS(l[0], "\xE7\xB9\x81\xE9\xAB\x94\xE4\xB8\xAD\xE6\x96\x87"); // 繁體中文
		TS_ASSERT_EQUALS(l[1], "\xE6\xB8\xAC\xE8\xA9\xA6");                         // 測試
		TS_ASSERT_EQUALS(remaining, Common::String(ZH_FANTI_TEST).size());
	}

	void test_no_punctuation_at_line_start() {
		TestFont font;
		unsigned int remaining;
		// 你好，世界 in a two character wide box
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font,
			"\xE4\xBD\xA0\xE5\xA5\xBD" ZH_COMMA "\xE4\xB8\x96\xE7\x95\x8C", 20, 0, remaining);
		for (uint n = 0; n < l.size(); n++)
			TS_ASSERT(!l[n].hasPrefix(ZH_COMMA));
		TS_ASSERT_EQUALS(l.size(), 3u);
		TS_ASSERT_EQUALS(l[1], "\xE5\xA5\xBD" ZH_COMMA); // 好，
	}

	void test_wrap_mixed_text_without_gap() {
		TestFont font;
		unsigned int remaining;
		// 我是在 Lurker 領域的深處: the CJK run after "Lurker " must wrap per character
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font,
			"\xE6\x88\x91\xE6\x98\xAF\xE5\x9C\xA8 Lurker \xE9\xA0\x98\xE5\x9F\x9F\xE7\x9A\x84\xE6\xB7\xB1\xE8\x99\x95",
			150, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 2u);
		TS_ASSERT_EQUALS(l[0], "\xE6\x88\x91\xE6\x98\xAF\xE5\x9C\xA8 Lurker \xE9\xA0\x98\xE5\x9F\x9F\xE7\x9A\x84\xE6\xB7\xB1");
		TS_ASSERT_EQUALS(l[1], "\xE8\x99\x95");
	}

	void test_u8_specials_in_utf8_text() {
		TestFont font;
		unsigned int remaining;
		// ~ is a line break
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font,
			"\xE7\xAC\xAC\xE4\xB8\x80\xE8\xA1\x8C~\xE7\xAC\xAC\xE4\xBA\x8C\xE8\xA1\x8C", 200, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 2u);
		TS_ASSERT_EQUALS(l[0], "\xE7\xAC\xAC\xE4\xB8\x80\xE8\xA1\x8C"); // 第一行
		TS_ASSERT_EQUALS(l[1], "\xE7\xAC\xAC\xE4\xBA\x8C\xE8\xA1\x8C"); // 第二行

		// * is a page break when paging
		Common::String paged("\xE7\x94\xB2*\xE4\xB9\x99"); // 甲*乙
		l = typeset<TestFont::UTF8>(font, paged, 200, 100, remaining, true);
		TS_ASSERT_EQUALS(l.size(), 1u);
		TS_ASSERT_EQUALS(l[0], "\xE7\x94\xB2");
		TS_ASSERT_EQUALS(remaining, 4u); // byte offset of 乙
	}

	void test_page_height_limit() {
		TestFont font;
		unsigned int remaining;
		// one character per line, room for two lines (12 + 10 <= 25)
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font, "\xE4\xB8\xAD\xE6\x96\x87\xE5\xAD\x97", 10, 25, remaining);
		TS_ASSERT_EQUALS(l.size(), 2u);
		TS_ASSERT_EQUALS(remaining, 6u); // 字 starts at byte 6
	}

	void test_very_narrow_rectangle() {
		TestFont font;
		unsigned int remaining;
		// narrower than a single character: still terminates and makes progress
		Common::Array<Common::String> l = typeset<TestFont::UTF8>(font, "\xE4\xB8\xAD\xE6\x96\x87\xE5\xAD\x97", 5, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 3u);
		TS_ASSERT_EQUALS(l[0], "\xE4\xB8\xAD");

		// the same used to loop forever for 8-bit text too
		TestFont legacy(false);
		l = typeset<TestFont::LegacyTraits>(legacy, "abc", 5, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 3u);
		TS_ASSERT_EQUALS(l[0], "a");
	}

	void test_legacy_traits_unchanged() {
		TestFont font(false);
		unsigned int remaining;
		Common::Array<Common::String> l = typeset<TestFont::LegacyTraits>(font, "hello world foo", 50, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 3u);
		TS_ASSERT_EQUALS(l[0], "hello");
		TS_ASSERT_EQUALS(l[1], "world");
		TS_ASSERT_EQUALS(l[2], "foo");

		l = typeset<TestFont::LegacyTraits>(font, "one~two", 100, 0, remaining);
		TS_ASSERT_EQUALS(l.size(), 2u);

		Common::String s("ab");
		Common::String::const_iterator i = s.begin();
		TS_ASSERT(!TestFont::LegacyTraits::endsWord(i));
		TS_ASSERT(!TestFont::SJIS::endsWord(i));
	}

	void test_sjis_traits_unchanged() {
		Common::String s("\x93\xfa\x96\x7b"); // 日本 in Shift-JIS
		TS_ASSERT_EQUALS(TestFont::SJIS::length(s), 2u);
		Common::String::const_iterator i = s.begin();
		TS_ASSERT_EQUALS(TestFont::SJIS::unicode(i), 0x65E5u);
		TS_ASSERT_EQUALS(TestFont::SJIS::unicode(i), 0x672Cu);
	}
};
