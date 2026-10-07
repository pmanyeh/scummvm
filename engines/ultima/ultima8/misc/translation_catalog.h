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

#ifndef ULTIMA8_MISC_TRANSLATION_CATALOG_H
#define ULTIMA8_MISC_TRANSLATION_CATALOG_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/str.h"

namespace Common {
class SeekableReadStream;
}

namespace Ultima {
namespace Ultima8 {

/**
 * Translations of the game text, compiled from PO files into one gettext MO
 * file. Every entry has a context that identifies where the text is shown:
 *
 *   bark CCCC:IIII   NPC speech: usecode class and IP of the bark intrinsic call
 *   book CCCC:IIII   book / scroll / gravestone / plaque text: class and IP of
 *   scroll, grave,   the Book::read, Scroll::read, Grave::read or Plaque::read
 *   plaque CCCC:IIII call
 *   ask CCCC         conversation answer: usecode class that called ask
 *   param CCCC:NAME  value of a sentence parameter (NAME = varXX or call_XXXX)
 *   ui               engine text (menus, labels), identified by its English text
 *
 * (hex, case-insensitive in the file). The msgid is the exact English text,
 * including trailing spaces; an entry is only used when it matches the text
 * the game is about to show.
 *
 * Barks that usecode builds from parts are sentence templates: the msgid
 * contains placeholders that match any text, and the translation uses the
 * same placeholders (in any order):
 *
 *   {name}       the Avatar's name (never translated)
 *   {num}        a number
 *   {varXX}      a usecode string variable; {call_XXXX} the result of a
 *                usecode function. Their values are translated with the
 *                "param CCCC:varXX" / "param CCCC:call_XXXX" entries of the
 *                same class if there are any, and shown as they are otherwise.
 */
class TranslationCatalog {
public:
	enum Result {
		kMiss,           //!< no entry (for an answer: no entry for this text)
		kHit,            //!< translation found
		kSourceMismatch  //!< bark call site known, but not with this English text
	};

	TranslationCatalog();

	void clear();

	/**
	 * Load a compiled catalog. Entries that are malformed (unknown context,
	 * empty or invalid UTF-8 translation, plurals, duplicates) are skipped.
	 * If the file structure itself is broken nothing is loaded.
	 * \return true if the file could be read
	 */
	bool load(Common::SeekableReadStream &rs, const Common::String &name);

	//! Look up a translation. translation is set only on kHit.
	Result lookup(const Common::String &context, const Common::String &source,
	              Common::String &translation) const;

	//! Any one entry of a context (for testing); false if there is none
	bool findContext(const Common::String &context, Common::String &source,
	                 Common::String &translation) const;

	uint size() const { return _entries.size(); }
	uint skipped() const { return _skipped; }
	//! Language from the catalog header (may be empty)
	const Common::String &language() const { return _language; }

	static Common::String barkContext(uint16 classId, uint16 ip);
	//! Context of a call site: kind is bark, book, scroll, grave or plaque
	static Common::String callSiteContext(const char *kind, uint16 classId, uint16 ip);
	static Common::String askContext(uint16 classId);

	//! Parse a context and write it in canonical form ("bark 0402:0633")
	static bool canonicalContext(const Common::String &context, Common::String &canonical);

	//! "varf7" -> "varF7", "call_0bcf" -> "call_0BCF", "part1" -> "part1";
	//! empty if not a parameter name
	static Common::String canonicalParam(const Common::String &name);

	//! True if the whole string is valid UTF-8
	static bool isValidUTF8(const Common::String &text);

	//! How much a CJK character counts in readingLength()
	static const uint CJK_READING_WEIGHT = 3;

	/**
	 * Length of text[start, end) for timing how long it is shown, in English
	 * letters: a CJK character counts CJK_READING_WEIGHT, any other
	 * character (UTF-8 or a byte of the game's code page) counts 1.
	 */
	static uint readingLength(const Common::String &text, uint start = 0,
	                          uint end = 0xFFFFFFFF);

	struct Entry {
		Common::String _context;
		Common::String _source;
		Common::String _translation;
	};
	//! All entries, in no particular order (for diagnostics)
	void listEntries(Common::Array<Entry> &entries) const;

	/**
	 * Placeholders of a template, in order (without braces).
	 * \return false if a brace does not start a known placeholder
	 */
	static bool placeholders(const Common::String &text, Common::Array<Common::String> &names);

	/**
	 * Match text against a template.
	 * \param values the text matched by each placeholder, in template order
	 */
	static bool matchTemplate(const Common::String &tmpl, const Common::String &text,
	                          Common::Array<Common::String> &values);

	//! Replace the placeholders of tmpl by the given values
	static Common::String fillTemplate(const Common::String &tmpl,
	                                   const Common::Array<Common::String> &names,
	                                   const Common::Array<Common::String> &values);

private:
	typedef Common::HashMap<Common::String, Common::String> EntryMap;
	typedef Common::HashMap<Common::String, bool> ContextSet;

	struct Template {
		Common::String _source;
		Common::String _translation;
		uint _literalLength;  //!< length of the fixed text of _source
	};
	typedef Common::HashMap<Common::String, Common::Array<Template> > TemplateMap;

	void addEntry(const Common::String &key, const Common::String &translation,
	              const Common::String &name);
	void parseHeader(const Common::String &header);
	bool lookupTemplate(const Common::String &context, const Common::String &source,
	                    Common::String &translation) const;

	EntryMap _entries;     //!< "<context>\x04<English>" -> translation
	ContextSet _barkContexts;  //!< call sites (bark, book, ...) with at least one entry
	TemplateMap _templates;    //!< bark call site -> its sentence templates
	uint _skipped;
	Common::String _language;
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
