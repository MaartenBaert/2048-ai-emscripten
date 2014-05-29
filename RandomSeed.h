/*
Copyright (c) 2014 Maarten Baert <maarten-baert@hotmail.com>

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#pragma once

#include <stdint.h>

#include <chrono>
#include <functional>

inline uint32_t RandomSeed() {
	// 64-bit FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/index.html)
	uint64_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	uint64_t h = UINT64_C(14695981039346656037);
	h ^= (time      ) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >>  8) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 16) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 24) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 32) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 40) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 48) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (time >> 56) & 0xff; h *= UINT64_C(1099511628211);
	return (uint32_t) h ^ (uint32_t) (h >> 32);
}
