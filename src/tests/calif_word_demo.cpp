#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../string_base.h"
#ifdef CALIF_ICU4X_DEMO
#include "../../demo/word_adapter.h"
#include <cstdlib>
#include <unicode/brkiter.h>
#include <unicode/unistr.h>

TEST_CASE("ICU4X matches ICU4C through OpenTTD StringIterator", "[calif][word]")
{
	const char *old = std::getenv("CALIF_WORD_BACKEND");
	const std::optional<std::string> previous = old ? std::optional<std::string>(old) : std::nullopt;
	struct Restore {
		const std::optional<std::string> &value;
		~Restore() { if (value) setenv("CALIF_WORD_BACKEND", value->c_str(), 1); else unsetenv("CALIF_WORD_BACKEND"); }
	} restore{previous};
	std::vector<std::string> corpus = {
		"", " ", "  hello  world  ", "Hello caf\u00E9 beautiful world", "don't stop... now!",
		"cafe\xcc\x81", "hello\tworld\n", "\u4E16\u754C \u4F60\u597D Rust", "\u0E20\u0E32\u0E29\u0E32\u0E44\u0E17\u0E22", "\U0001F469\u200D\U0001F4BB hello \U0001F682 world",
		"a\xc2\xa0" "b", "alpha-beta_42.test", "\r\n", "a\xe2\x80\x8b" "b"
	};
	uint32_t seed = 0xCA11F;
	const std::string tokens[] = {"a", " ", "caf\u00E9", "\u4E16\u754C", "\U0001F682", "...", "\t", "e\xcc\x81", "_", "42"};
	for (unsigned n = 0; n < 256; ++n) {
		std::string s;
		for (unsigned k = 0; k < 12; ++k) { seed = 1664525 * seed + 1013904223; s += tokens[seed % std::size(tokens)]; }
		corpus.push_back(s);
	}
	setenv("CALIF_WORD_BACKEND", "icu4c", 1);
	auto icu = StringIterator::Create();
	setenv("CALIF_WORD_BACKEND", "icu4x", 1);
	auto rust = StringIterator::Create();
	const auto calls_before = Icu4xWordIterator::DemoSegmentationCalls();
	for (const auto &text : corpus) {
		INFO("input: " << text);
		icu->SetString(text);
		rust->SetString(text);
		std::vector<size_t> positions{0};
		for (size_t p = icu->Next(); p != StringIterator::END; p = icu->Next()) positions.push_back(p);
		for (size_t p : positions) {
			INFO("cursor byte: " << p);
			REQUIRE(icu->SetCurPosition(p) == rust->SetCurPosition(p));
			CHECK(icu->Next(StringIterator::ITER_WORD) == rust->Next(StringIterator::ITER_WORD));
			REQUIRE(icu->SetCurPosition(p) == rust->SetCurPosition(p));
			CHECK(icu->Prev(StringIterator::ITER_WORD) == rust->Prev(StringIterator::ITER_WORD));
		}
	}
	CHECK(Icu4xWordIterator::DemoSegmentationCalls() - calls_before == corpus.size());
}

TEST_CASE("Rust word cursor preserves ICU4C navigation state", "[calif][word]")
{
	UErrorCode status = U_ZERO_ERROR;
	std::unique_ptr<icu::BreakIterator> icu(icu::BreakIterator::createWordInstance(icu::Locale("en"), status));
	REQUIRE(U_SUCCESS(status));
	REQUIRE(icu != nullptr);
	Icu4xWordIterator rust;
	// Reuse the cursor across growth, shrinking, and empty input. The binding
	// must retain boundaries independently of the temporary input buffer.
	const std::u16string inputs[] = {u"", u"Hello caf\u00E9 beautiful world", u"a", u"\U0001F682 hello \u4E16\u754C", u"  ", u""};
	for (const auto &text : inputs) {
		INFO("UTF-16 length: " << text.size());
		icu::UnicodeString baseline_text(text.data(), static_cast<int32_t>(text.size()));
		icu->setText(baseline_text);
		{
			std::u16string temporary = text;
			REQUIRE(rust.SetText(temporary));
		}
		CHECK(rust.first() == icu->first());
		for (size_t i = 0; i < text.size() + 3; ++i) CHECK(rust.next() == icu->next());
		for (size_t i = 0; i < text.size() + 3; ++i) CHECK(rust.previous() == icu->previous());
		for (int32_t pos = -1; pos <= static_cast<int32_t>(text.size()) + 1; ++pos) {
			// OpenTTD passes code point boundaries, never the middle of a surrogate pair.
			if (pos > 0 && pos < static_cast<int32_t>(text.size()) && text[pos] >= 0xDC00 && text[pos] <= 0xDFFF) continue;
			INFO("seek position: " << pos);
			CHECK(rust.following(pos) == icu->following(pos));
			int32_t previous = icu->previous();
			CHECK(rust.previous() == previous);
			// Reset after backward exhaustion. ICU4C 78's rule cache can return
			// DONE on a subsequent next() while advancing current(); that
			// internal cache behavior isn't used by OpenTTD's caller.
			if (previous == icu::BreakIterator::DONE) CHECK(rust.first() == icu->first());
			CHECK(rust.next() == icu->next());
			int32_t preceding = icu->preceding(pos);
			CHECK(rust.preceding(pos) == preceding);
			if (preceding == icu::BreakIterator::DONE) CHECK(rust.first() == icu->first());
			CHECK(rust.next() == icu->next());
			CHECK(rust.previous() == icu->previous());
		}
	}
}
#endif
