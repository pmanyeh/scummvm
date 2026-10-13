#include <cxxtest/TestSuite.h>
#include "common/array.h"
#include "common/memstream.h"
#include "engines/ultima/ultima8/misc/translation_catalog.h"

// UTF-8 written as escapes; adjacent literals keep escapes apart from the
// following text ("\x04" "Hello").
#define ZH_HELLO "\xE4\xBD\xA0\xE5\xA5\xBD\xE5\x95\x8A\xE3\x80\x82"   // 你好啊。
#define ZH_WHO   "\xE4\xBD\xA0\xE6\x98\xAF\xE8\xAA\xB0\xEF\xBC\x9F"   // 你是誰？
#define ZH_BYE   "\xE5\x86\x8D\xE8\xA6\x8B\xE3\x80\x82"               // 再見。

namespace {

using Ultima::Ultima8::TranslationCatalog;

struct MOEntry {
	Common::String _key;
	Common::String _value;
	MOEntry(const Common::String &k, const Common::String &v) : _key(k), _value(v) {}
};

void put32(Common::Array<byte> &out, uint32 offset, uint32 v, bool bigEndian) {
	for (int i = 0; i < 4; i++) {
		int shift = bigEndian ? 24 - 8 * i : 8 * i;
		out[offset + i] = (v >> shift) & 0xFF;
	}
}

// Minimal gettext MO writer (no hash table), like msgfmt output
Common::Array<byte> buildMO(const Common::Array<MOEntry> &entries, bool bigEndian = false) {
	uint32 n = entries.size();
	uint32 origTable = 28, transTable = origTable + 8 * n, strings = transTable + 8 * n;
	Common::Array<byte> out;
	out.resize(strings);
	put32(out, 0, 0x950412de, bigEndian);
	put32(out, 4, 0, bigEndian);
	put32(out, 8, n, bigEndian);
	put32(out, 12, origTable, bigEndian);
	put32(out, 16, transTable, bigEndian);
	put32(out, 20, 0, bigEndian);
	put32(out, 24, strings, bigEndian);

	for (int pass = 0; pass < 2; pass++) {
		for (uint32 i = 0; i < n; i++) {
			const Common::String &s = pass == 0 ? entries[i]._key : entries[i]._value;
			uint32 table = pass == 0 ? origTable : transTable;
			uint32 off = out.size();
			for (uint j = 0; j < s.size(); j++)
				out.push_back(s[j]);
			out.push_back(0);
			put32(out, table + 8 * i, s.size(), bigEndian);
			put32(out, table + 8 * i + 4, off, bigEndian);
		}
	}
	return out;
}

bool loadMO(TranslationCatalog &cat, const Common::Array<byte> &data) {
	Common::MemoryReadStream rs(data.data(), data.size());
	return cat.load(rs, "test.mo");
}

Common::String key(const char *context, const char *english) {
	Common::String k(context);
	k += '\x04';
	k += english;
	return k;
}

Common::Array<MOEntry> devonEntries() {
	Common::Array<MOEntry> e;
	e.push_back(MOEntry("", "Content-Type: text/plain; charset=UTF-8\nLanguage: zh_TW\n"));
	e.push_back(MOEntry(key("bark 0402:0633", "Hello there. "), ZH_HELLO));
	e.push_back(MOEntry(key("ask 0402", "Who are you? "), ZH_WHO));
	e.push_back(MOEntry(key("ask 0402", "Goodbye. "), ZH_BYE));
	return e;
}

} // End of anonymous namespace

class U8TranslationCatalogTestSuite : public CxxTest::TestSuite {
public:
	void test_context_format() {
		TS_ASSERT_EQUALS(TranslationCatalog::barkContext(0x402, 0x633), "bark 0402:0633");
		TS_ASSERT_EQUALS(TranslationCatalog::barkContext(0x40A, 0x0BCF), "bark 040A:0BCF");
		TS_ASSERT_EQUALS(TranslationCatalog::askContext(0x402), "ask 0402");
	}

	void test_canonical_context() {
		Common::String c;
		TS_ASSERT(TranslationCatalog::canonicalContext("bark 040a:0bcf", c));
		TS_ASSERT_EQUALS(c, "bark 040A:0BCF");
		TS_ASSERT(TranslationCatalog::canonicalContext("ask 402", c));
		TS_ASSERT_EQUALS(c, "ask 0402");
		TS_ASSERT(!TranslationCatalog::canonicalContext("bark 0402", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("bark 0402:", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("bark 0402:12345", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("ask 04G2", c));
		// one frame of the barking item
		TS_ASSERT(TranslationCatalog::canonicalContext("bark 2fe:9a#05", c));
		TS_ASSERT_EQUALS(c, "bark 02FE:009A#5");
		TS_ASSERT(!TranslationCatalog::canonicalContext("bark 02FE:009A#", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("bark 02FE:009A#x", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("book 02FE:009A#1", c));
		TS_ASSERT(TranslationCatalog::canonicalContext("param 40a:varf7", c));
		TS_ASSERT_EQUALS(c, "param 040A:varF7");
		TS_ASSERT(TranslationCatalog::canonicalContext("param 040A:call_bcf", c));
		TS_ASSERT_EQUALS(c, "param 040A:call_0BCF");
		TS_ASSERT(!TranslationCatalog::canonicalContext("param 040A:name", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("param 040A", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("talk 0402", c));
		TS_ASSERT(TranslationCatalog::canonicalContext("book 25f:46d", c));
		TS_ASSERT_EQUALS(c, "book 025F:046D");
		TS_ASSERT(TranslationCatalog::canonicalContext("scroll 0579:0168", c));
		TS_ASSERT(TranslationCatalog::canonicalContext("grave 00C5:0118", c));
		TS_ASSERT(TranslationCatalog::canonicalContext("plaque 0257:0109", c));
		TS_ASSERT_EQUALS(c, "plaque 0257:0109");
		TS_ASSERT(!TranslationCatalog::canonicalContext("book 025F", c));
		TS_ASSERT_EQUALS(TranslationCatalog::callSiteContext("grave", 0xC5, 0x118), "grave 00C5:0118");
		TS_ASSERT(TranslationCatalog::canonicalContext("ui", c));
		TS_ASSERT_EQUALS(c, "ui");
		TS_ASSERT(!TranslationCatalog::canonicalContext("ui 1", c));
		TS_ASSERT(!TranslationCatalog::canonicalContext("", c));
	}

	void test_utf8_validation() {
		TS_ASSERT(TranslationCatalog::isValidUTF8(ZH_HELLO));
		TS_ASSERT(TranslationCatalog::isValidUTF8("plain ASCII"));
		TS_ASSERT(!TranslationCatalog::isValidUTF8("hei\xE1" "e"));   // CP437 text
		TS_ASSERT(!TranslationCatalog::isValidUTF8("\xE4\xBD"));       // truncated
	}

	void test_hit() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries())));
		TS_ASSERT_EQUALS(cat.size(), 3u);
		TS_ASSERT_EQUALS(cat.skipped(), 0u);
		TS_ASSERT_EQUALS(cat.language(), "zh_TW");

		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == ZH_HELLO);
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Who are you? ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == ZH_WHO);
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Goodbye. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == ZH_BYE);
	}

	void test_miss_and_mismatch() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries())));

		Common::String t;
		// unknown call site
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0634", "Hello there. ", t), TranslationCatalog::kMiss);
		// unknown class asking the same question
		TS_ASSERT_EQUALS(cat.lookup("ask 0403", "Who are you? ", t), TranslationCatalog::kMiss);
		// known call site, different text (dynamic text or another game version)
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there, Avatar. ", t),
		                 TranslationCatalog::kSourceMismatch);
		// an answer the catalog does not have is a miss, not a mismatch
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Hello, Devon. ", t), TranslationCatalog::kMiss);
		// comparison is exact: case and trailing space matter, as in usecode
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there.", t), TranslationCatalog::kSourceMismatch);
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "who are you? ", t), TranslationCatalog::kMiss);
	}

	void test_several_sources_per_bark() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("bark 0402:1000", "Yes. "), "A"));
		e.push_back(MOEntry(key("bark 0402:1000", "No. "), "B"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));

		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:1000", "Yes. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == "A");
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:1000", "No. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == "B");
	}

	void test_find_context() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries())));
		Common::String source, translation;
		TS_ASSERT(cat.findContext("bark 402:633", source, translation));
		TS_ASSERT_EQUALS(source, "Hello there. ");
		TS_ASSERT_EQUALS(translation, ZH_HELLO);
		TS_ASSERT(cat.findContext("ask 0402", source, translation));
		TS_ASSERT(!cat.findContext("bark 0402:0634", source, translation));
		TS_ASSERT(!cat.findContext("bark 0402", source, translation));
	}

	void test_placeholders() {
		Common::Array<Common::String> n;
		TS_ASSERT(TranslationCatalog::placeholders("Hello there, {name}.", n));
		TS_ASSERT_EQUALS(n.size(), 1u);
		TS_ASSERT_EQUALS(n[0], "name");
		TS_ASSERT(TranslationCatalog::placeholders("{varF7} and {call_0BCF} x{num}", n));
		TS_ASSERT_EQUALS(n.size(), 3u);
		TS_ASSERT_EQUALS(n[1], "call_0BCF");
		TS_ASSERT(TranslationCatalog::placeholders("no placeholders", n));
		TS_ASSERT_EQUALS(n.size(), 0u);
		TS_ASSERT(!TranslationCatalog::placeholders("{nme}", n));
		TS_ASSERT(!TranslationCatalog::placeholders("{name", n));
		TS_ASSERT(!TranslationCatalog::placeholders("name}", n));
		TS_ASSERT(!TranslationCatalog::placeholders("{varf7}", n));   // canonical hex only
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("varf7"), "varF7");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("call_bcf"), "call_0BCF");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("name"), "");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part1"), "part1");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part12"), "part12");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part0"), "");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part01"), "");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part123"), "");
		TS_ASSERT_EQUALS(TranslationCatalog::canonicalParam("part"), "");
		TS_ASSERT(TranslationCatalog::placeholders("{part1}of {part2}", n));
		TS_ASSERT_EQUALS(n.size(), 2u);
		Common::String c;
		TS_ASSERT(TranslationCatalog::canonicalContext("param 18d:part2", c));
		TS_ASSERT_EQUALS(c, "param 018D:part2");
	}

	void test_match_template() {
		Common::Array<Common::String> v;
		TS_ASSERT(TranslationCatalog::matchTemplate("Hello there, {name}.", "Hello there, Pman.", v));
		TS_ASSERT_EQUALS(v.size(), 1u);
		TS_ASSERT_EQUALS(v[0], "Pman");
		// a name containing the following literal text
		TS_ASSERT(TranslationCatalog::matchTemplate("Bye {name}. ", "Bye Dr. Who. ", v));
		TS_ASSERT_EQUALS(v[0], "Dr. Who");
		TS_ASSERT(TranslationCatalog::matchTemplate("{varF7} wants {num} {call_0BCF}.",
		                                            "stranger  wants 3 ale.", v));
		TS_ASSERT_EQUALS(v.size(), 3u);
		TS_ASSERT_EQUALS(v[0], "stranger ");
		TS_ASSERT_EQUALS(v[1], "3");
		TS_ASSERT_EQUALS(v[2], "ale");
		TS_ASSERT(!TranslationCatalog::matchTemplate("Hello there, {name}.", "Hello there, Pman!", v));
		TS_ASSERT(!TranslationCatalog::matchTemplate("Hello there, {name}.", "Hello there, .", v));
		TS_ASSERT(!TranslationCatalog::matchTemplate("Hello there, {name}.", "Hi there, Pman.", v));

		Common::Array<Common::String> names;
		names.push_back("name");
		names.push_back("num");
		Common::Array<Common::String> values;
		values.push_back("Pman");
		values.push_back("2");
		TS_ASSERT_EQUALS(TranslationCatalog::fillTemplate("{num} for {name}, {name}!", names, values),
		                 "2 for Pman, Pman!");
	}

	void test_template_lookup() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("bark 0402:0964", "Glad to see you, {name}."), "{name}" ZH_HELLO));
		e.push_back(MOEntry(key("bark 040A:0100", "Greetings again {varF7}. Another {call_0BCF}?"),
		                    "{call_0BCF}/{varF7}"));
		e.push_back(MOEntry(key("param 040a:varf7", "stranger "), "X"));
		// translation with a placeholder the source does not have
		e.push_back(MOEntry(key("bark 0402:0999", "Bye {name}."), "{num}"));
		// translation that drops a placeholder
		e.push_back(MOEntry(key("bark 0402:0998", "Bye {name}."), "Bye."));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		TS_ASSERT_EQUALS(cat.skipped(), 2u);

		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0964", "Glad to see you, Pman.", t), TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "Pman" ZH_HELLO);
		// parameter values: translated if the class has an entry, kept otherwise
		TS_ASSERT_EQUALS(cat.lookup("bark 040A:0100", "Greetings again stranger . Another ale?", t),
		                 TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "ale/X");
		TS_ASSERT_EQUALS(cat.lookup("bark 040A:0100", "Greetings again Pman. Another mead?", t),
		                 TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "mead/Pman");
		// text that does not fit the template
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0964", "Something else.", t), TranslationCatalog::kSourceMismatch);
		// templates are per call site
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0965", "Glad to see you, Pman.", t), TranslationCatalog::kMiss);
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0999", "Bye Pman.", t), TranslationCatalog::kMiss);
	}

	// Timing counts letters, not bytes: CJK characters count 3
	void test_reading_length() {
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("Hello there. "), 13u);
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength(ZH_HELLO), 4u * 3u);
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("Pman" ZH_HELLO), 4u + 4u * 3u);
		// full-width punctuation counts as CJK, other UTF-8 as one letter
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("\xEF\xBC\x81"), 3u);
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("caf\xC3\xA9"), 4u);
		// the game's own code page 437 bytes count one each
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("M\x81nchen"), 7u);
		// a part of the text
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("ab" ZH_HELLO "cd", 2, 2 + 3), 3u);
		TS_ASSERT_EQUALS(TranslationCatalog::readingLength("abc", 1), 2u);
	}

	// Answers built with the player's name ("I am " + name + ".")
	void test_answer_template() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("ask 0404", "I am {name}."), "X{name}Y"));
		e.push_back(MOEntry(key("ask 0404", "Bye "), "B"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("ask 0404", "I am Pman.", t), TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "XPmanY");
		TS_ASSERT_EQUALS(cat.lookup("ask 0404", "Bye ", t), TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "B");
		TS_ASSERT_EQUALS(cat.lookup("ask 0404", "I am the Avatar! ", t), TranslationCatalog::kMiss);
		// answers are per class
		TS_ASSERT_EQUALS(cat.lookup("ask 0408", "I am Pman.", t), TranslationCatalog::kMiss);
	}

	// Names get their English the first time they are shown
	void test_annotate_terms() {
#define T_A "\xE7\x94\xB2"      // two names: A B = "Alpha", B = "Beta",
#define T_B "\xE4\xB9\x99"      // A B C = a longer name without annotation
#define T_C "\xE4\xB8\x99"
#define T_COMMA "\xEF\xBC\x8C"
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("term", T_A T_B), "Alpha"));
		e.push_back(MOEntry(key("term", T_A T_B T_C), "-"));
		e.push_back(MOEntry(key("term", T_B), "Beta"));
		e.push_back(MOEntry(key("term", "\xE4"), "Broken"));    // not UTF-8
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		TS_ASSERT_EQUALS(cat.termCount(), 3u);
		TS_ASSERT_EQUALS(cat.skipped(), 1u);
		TS_ASSERT_EQUALS(cat.size(), 0u);     // terms are not translations

		Common::HashMap<Common::String, bool> shown;
		TS_ASSERT_EQUALS(cat.annotateTerms(T_A T_B T_COMMA T_A T_B T_C T_COMMA T_B T_COMMA T_A T_B, shown),
		                 T_A T_B "(Alpha)" T_COMMA T_A T_B T_C T_COMMA T_B "(Beta)" T_COMMA T_A T_B);
		// the same conversation: already shown
		TS_ASSERT_EQUALS(cat.annotateTerms(T_B T_A T_B, shown), T_B T_A T_B);
		// a new one; English already written by the translator is not repeated
		Common::HashMap<Common::String, bool> other;
		TS_ASSERT_EQUALS(cat.annotateTerms(T_A T_B "(Alpha)" T_B, other), T_A T_B "(Alpha)" T_B "(Beta)");
		TS_ASSERT(other.contains(T_A T_B));
		// text without names, English text
		TS_ASSERT_EQUALS(cat.annotateTerms("Hello there. ", other), "Hello there. ");
		TS_ASSERT_EQUALS(cat.annotateTerms("", other), "");
#undef T_A
#undef T_B
#undef T_C
#undef T_COMMA
	}

	// A sentence combined from pieces (FIREITEM): several templates of one
	// call site match; the one with the most fixed text wins
	void test_part_templates() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("bark 018D:0462", "{part1}of {part2}"), "{part2}{part1}"));
		e.push_back(MOEntry(key("bark 018D:0462", "{part1}of {part2}with {num} uses remaining"),
		                    "{part2}{part1}/{num}"));
		e.push_back(MOEntry(key("bark 018D:0462", "wand "), "W"));
		e.push_back(MOEntry(key("param 018D:part1", "wand "), "W"));
		e.push_back(MOEntry(key("param 018D:part2", "armor of flames "), "A"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		TS_ASSERT_EQUALS(cat.skipped(), 0u);

		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("bark 018D:0462", "wand of armor of flames with 3 uses remaining", t),
		                 TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "AW/3");
		TS_ASSERT_EQUALS(cat.lookup("bark 018D:0462", "wand of armor of flames ", t),
		                 TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "AW");
		// a piece without translation stays English
		TS_ASSERT_EQUALS(cat.lookup("bark 018D:0462", "rod of ignite ", t), TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "ignite rod ");
		// a whole sentence entry comes before the templates
		TS_ASSERT_EQUALS(cat.lookup("bark 018D:0462", "wand ", t), TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "W");
	}

	void test_readers() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("grave 00C5:0118", "SIVLE*THE MINSTREL*WANDERS NO MORE"), "X"));
		e.push_back(MOEntry(key("book 025F:046D", "Journal"), "Y"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("grave 00C5:0118", "SIVLE*THE MINSTREL*WANDERS NO MORE", t),
		                 TranslationCatalog::kHit);
		TS_ASSERT_EQUALS(t, "X");
		// a call site showing other text than the catalog knows
		TS_ASSERT_EQUALS(cat.lookup("book 025F:046D", "Other text", t), TranslationCatalog::kSourceMismatch);
		// kinds are separate: the same id as a bark is unknown
		TS_ASSERT_EQUALS(cat.lookup("bark 025F:046D", "Journal", t), TranslationCatalog::kMiss);
	}

	void test_lowercase_hex_in_file() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("bark 040a:0bcf", "Ale. "), "X"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("bark 040A:0BCF", "Ale. ", t), TranslationCatalog::kHit);
	}

	void test_malformed_entries_skipped() {
		Common::Array<MOEntry> e = devonEntries();
		e.push_back(MOEntry("Hello there. ", ZH_HELLO));                          // no context
		e.push_back(MOEntry(key("talk 0402:0633", "Hi. "), ZH_HELLO));            // unknown kind
		e.push_back(MOEntry(key("bark 04X2:0633", "Hi. "), ZH_HELLO));            // bad hex
		e.push_back(MOEntry(key("bark 0402:0700", "Hi. "), ""));                  // untranslated
		e.push_back(MOEntry(key("bark 0402:0701", ""), ZH_HELLO));                // empty source
		e.push_back(MOEntry(key("bark 0402:0702", "Hi. "), "hei\xE1" "e"));       // not UTF-8
		Common::String plural("A");
		plural += '\0';
		plural += "B";
		e.push_back(MOEntry(key("bark 0402:0703", "Hi. "), plural));              // plural forms
		e.push_back(MOEntry(key("bark 0402:0633", "Hello there. "), "duplicate"));

		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		TS_ASSERT_EQUALS(cat.size(), 3u);
		TS_ASSERT_EQUALS(cat.skipped(), 8u);

		Common::String t;
		// the first entry wins over the duplicate
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t == ZH_HELLO);
		// skipped entries leave English text
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0700", "Hi. ", t), TranslationCatalog::kMiss);
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0702", "Hi. ", t), TranslationCatalog::kMiss);
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0703", "Hi. ", t), TranslationCatalog::kMiss);
	}

	void test_big_endian_file() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries(), true)));
		TS_ASSERT_EQUALS(cat.size(), 3u);
		Common::String t;
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Who are you? ", t), TranslationCatalog::kHit);
	}

	void test_corrupt_files_rejected() {
		TranslationCatalog cat;

		// not a catalog at all
		Common::Array<byte> text;
		const char *tsv = "bark\t0402:0633\tHello there. \txx\n# padding to 28 bytes...";
		for (const char *p = tsv; *p; p++)
			text.push_back(*p);
		TS_ASSERT(!loadMO(cat, text));
		TS_ASSERT_EQUALS(cat.size(), 0u);

		// too short
		Common::Array<byte> tiny = buildMO(devonEntries());
		tiny.resize(20);
		TS_ASSERT(!loadMO(cat, tiny));

		// truncated: string data missing
		Common::Array<byte> cut = buildMO(devonEntries());
		cut.resize(cut.size() - 10);
		TS_ASSERT(!loadMO(cat, cut));
		TS_ASSERT_EQUALS(cat.size(), 0u);

		// entry count larger than the tables
		Common::Array<byte> count = buildMO(devonEntries());
		put32(count, 8, 1000000, false);
		TS_ASSERT(!loadMO(cat, count));

		// string offset pointing past the end
		Common::Array<byte> offset = buildMO(devonEntries());
		put32(offset, 28 + 8 * 4 + 8 + 4, 0xFFFFFFF0, false);
		TS_ASSERT(!loadMO(cat, offset));
		TS_ASSERT_EQUALS(cat.size(), 0u);

		// a later good load still works
		TS_ASSERT(loadMO(cat, buildMO(devonEntries())));
		TS_ASSERT_EQUALS(cat.size(), 3u);
	}
};
