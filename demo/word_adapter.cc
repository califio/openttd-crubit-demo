#include "word_adapter.h"
#include <atomic>
#include <vector>
#include "support/annotations.h"
#include "crubit/icu4x_word_bindings.h"

namespace {
// Recording instrumentation; not part of the reusable cursor API.
std::atomic<size_t> demo_segmentation_calls{0};
}

struct Icu4xWordIterator::Impl {
	icu4x_word_bindings::WordIterator cursor;
	std::vector<uint16_t> units;
};
Icu4xWordIterator::Icu4xWordIterator() : impl(std::make_unique<Impl>()) {}
Icu4xWordIterator::~Icu4xWordIterator() = default;
bool Icu4xWordIterator::SetText(std::span<const char16_t> text) {
	// char16_t and uint16_t are distinct C++ types; don't alias them.
	// Keep this type conversion here; Rust owns all cursor behavior.
	impl->units.assign(text.begin(), text.end());
	if (!impl->cursor.set_text(rs_std::SliceRef<const uint16_t>(impl->units))) return false;
	demo_segmentation_calls.fetch_add(1, std::memory_order_relaxed);
	return true;
}
int32_t Icu4xWordIterator::first() { return impl->cursor.first(); }
int32_t Icu4xWordIterator::following(int32_t pos) { return impl->cursor.following(pos); }
int32_t Icu4xWordIterator::preceding(int32_t pos) { return impl->cursor.preceding(pos); }
int32_t Icu4xWordIterator::next() { return impl->cursor.next(); }
int32_t Icu4xWordIterator::previous() { return impl->cursor.previous(); }
size_t Icu4xWordIterator::DemoSegmentationCalls() {
	return demo_segmentation_calls.load(std::memory_order_relaxed);
}
