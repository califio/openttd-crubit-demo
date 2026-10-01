#pragma once
#include <cstdint>
#include <memory>
#include <span>

// C++ forwarding shim for the Rust-owned ICU4C-style word cursor.
// Isolates Crubit headers in a translation unit compiled without exceptions.
// All boundary storage, searching, and cursor movement live in Rust.
class Icu4xWordIterator {
	struct Impl;
	std::unique_ptr<Impl> impl;
public:
	Icu4xWordIterator();
	~Icu4xWordIterator();
	bool SetText(std::span<const char16_t> text);
	int32_t first();
	int32_t following(int32_t pos);
	int32_t preceding(int32_t pos);
	int32_t next();
	int32_t previous();
	static size_t DemoSegmentationCalls();
};
