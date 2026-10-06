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
		TS_ASSERT(!TranslationCatalog::canonicalContext("param 040A:varF7", c));
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

		const Common::String *t;
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == ZH_HELLO);
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Who are you? ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == ZH_WHO);
		TS_ASSERT_EQUALS(cat.lookup("ask 0402", "Goodbye. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == ZH_BYE);
	}

	void test_miss_and_mismatch() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries())));

		const Common::String *t;
		// unknown call site
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0634", "Hello there. ", t), TranslationCatalog::kMiss);
		TS_ASSERT(t == nullptr);
		// unknown class asking the same question
		TS_ASSERT_EQUALS(cat.lookup("ask 0403", "Who are you? ", t), TranslationCatalog::kMiss);
		// known call site, different text (dynamic text or another game version)
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there, Avatar. ", t),
		                 TranslationCatalog::kSourceMismatch);
		TS_ASSERT(t == nullptr);
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

		const Common::String *t;
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:1000", "Yes. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == "A");
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:1000", "No. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == "B");
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

	void test_lowercase_hex_in_file() {
		Common::Array<MOEntry> e;
		e.push_back(MOEntry(key("bark 040a:0bcf", "Ale. "), "X"));
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(e)));
		const Common::String *t;
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

		const Common::String *t;
		// the first entry wins over the duplicate
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0633", "Hello there. ", t), TranslationCatalog::kHit);
		TS_ASSERT(t && *t == ZH_HELLO);
		// skipped entries leave English text
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0700", "Hi. ", t), TranslationCatalog::kMiss);
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0702", "Hi. ", t), TranslationCatalog::kMiss);
		TS_ASSERT_EQUALS(cat.lookup("bark 0402:0703", "Hi. ", t), TranslationCatalog::kMiss);
	}

	void test_big_endian_file() {
		TranslationCatalog cat;
		TS_ASSERT(loadMO(cat, buildMO(devonEntries(), true)));
		TS_ASSERT_EQUALS(cat.size(), 3u);
		const Common::String *t;
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
