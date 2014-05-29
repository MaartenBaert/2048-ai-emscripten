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

#include "PerfTests.h"

#include "Board.h"

#include <chrono>
#include <iostream>
#include <random>

#define CPU_FREQ_MHZ 3890

#define TEST_COLLAPSE_ROW(CODE) { \
	unsigned int hash = 2166136261; \
	std::mt19937 rng(12345); \
	auto t1 = std::chrono::high_resolution_clock::now(); \
	unsigned int loops = 10000000; \
	for(unsigned int i = 0; i < loops; ++i) { \
		BoardRow row{(uint16_t) rng()}; \
		BoardRowScore result = CODE; \
		hash ^= result.m_row.m_data; \
		hash *= 16777619; \
		hash ^= result.m_score; \
		hash *= 16777619; \
	} \
	auto t2 = std::chrono::high_resolution_clock::now(); \
	uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); \
	std::cout << "    time=" << CPU_FREQ_MHZ * time / (loops / 1000) << " mcy, hash=" << hash << std::endl; \
}

void Test_CollapseRow() {
	std::cout << "**** Test_CollapseRow ****" << std::endl;
	std::cout << "Reference:" << std::endl;
	TEST_COLLAPSE_ROW((BoardRowScore{row, 0}));
	TEST_COLLAPSE_ROW((BoardRowScore{row, 0}));
	std::cout << "SlowCollapseLeft:" << std::endl;
	TEST_COLLAPSE_ROW(SlowCollapseLeft(row));
	TEST_COLLAPSE_ROW(SlowCollapseLeft(row));
	std::cout << "CollapseLeft:" << std::endl;
	TEST_COLLAPSE_ROW(CollapseLeft(row));
	TEST_COLLAPSE_ROW(CollapseLeft(row));
	std::cout << "CollapseRight1:" << std::endl;
	TEST_COLLAPSE_ROW(CollapseRight(row));
	TEST_COLLAPSE_ROW(CollapseRight(row));
	std::cout << "CollapseRight2:" << std::endl;
	TEST_COLLAPSE_ROW(CollapseRight2(row));
	TEST_COLLAPSE_ROW(CollapseRight2(row));
	std::cout << "----------------------------------------" << std::endl;
}

#define TEST_FLIP_BOARD(CODE) { \
	unsigned int hash = 2166136261; \
	std::mt19937_64 rng(12345); \
	auto t1 = std::chrono::high_resolution_clock::now(); \
	unsigned int loops = 10000000; \
	for(unsigned int i = 0; i < loops; ++i) { \
		Board board{rng()}; \
		board = CODE; \
		hash ^= GetRow(board, 0).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 1).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 2).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 3).m_data; \
		hash *= 16777619; \
	} \
	auto t2 = std::chrono::high_resolution_clock::now(); \
	uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); \
	std::cout << "    time=" << CPU_FREQ_MHZ * time / (loops / 1000) << " mcy, hash=" << hash << std::endl; \
}

void Test_FlipBoard() {
	std::cout << "**** Test_FlipBoard ****" << std::endl;
	std::cout << "Reference:" << std::endl;
	TEST_FLIP_BOARD(board);
	TEST_FLIP_BOARD(board);
	std::cout << "FlipLR:" << std::endl;
	TEST_FLIP_BOARD(FlipLR(board));
	TEST_FLIP_BOARD(FlipLR(board));
	std::cout << "FlipUD:" << std::endl;
	TEST_FLIP_BOARD(FlipUD(board));
	TEST_FLIP_BOARD(FlipUD(board));
	std::cout << "Transpose:" << std::endl;
	TEST_FLIP_BOARD(Transpose(board));
	TEST_FLIP_BOARD(Transpose(board));
	std::cout << "----------------------------------------" << std::endl;
}

#define TEST_COLLAPSE_BOARD(CODE) { \
	unsigned int hash = 2166136261; \
	std::mt19937_64 rng(12345); \
	auto t1 = std::chrono::high_resolution_clock::now(); \
	unsigned int loops = 10000000; \
	for(unsigned int i = 0; i < loops; ++i) { \
		Board board{rng()}; \
		BoardScore result = CODE; \
		hash ^= GetRow(result.m_board, 0).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(result.m_board, 1).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(result.m_board, 2).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(result.m_board, 3).m_data; \
		hash *= 16777619; \
		hash ^= result.m_score; \
		hash *= 16777619; \
	} \
	auto t2 = std::chrono::high_resolution_clock::now(); \
	uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); \
	std::cout << "    time=" << CPU_FREQ_MHZ * time / (loops / 1000) << " mcy, hash=" << hash << std::endl; \
}

void Test_CollapseBoard() {
	std::cout << "**** Test_CollapseBoard ****" << std::endl;
	std::cout << "Reference:" << std::endl;
	TEST_COLLAPSE_BOARD((BoardScore{board, 0}));
	TEST_COLLAPSE_BOARD((BoardScore{board, 0}));
	std::cout << "CollapseLeft:" << std::endl;
	TEST_COLLAPSE_BOARD(CollapseLeft(board));
	TEST_COLLAPSE_BOARD(CollapseLeft(board));
	std::cout << "CollapseRight:" << std::endl;
	TEST_COLLAPSE_BOARD(CollapseRight(board));
	TEST_COLLAPSE_BOARD(CollapseRight(board));
	std::cout << "CollapseUp:" << std::endl;
	TEST_COLLAPSE_BOARD(CollapseUp(board));
	TEST_COLLAPSE_BOARD(CollapseUp(board));
	std::cout << "CollapseDown:" << std::endl;
	TEST_COLLAPSE_BOARD(CollapseDown(board));
	TEST_COLLAPSE_BOARD(CollapseDown(board));
	std::cout << "----------------------------------------" << std::endl;
}

#define TEST_NORMALIZE_BOARD(CODE) { \
	unsigned int hash = 2166136261; \
	std::mt19937_64 rng(12345); \
	auto t1 = std::chrono::high_resolution_clock::now(); \
	unsigned int loops = 10000000; \
	for(unsigned int i = 0; i < loops; ++i) { \
		Board board{rng()}; \
		board = CODE; \
		hash ^= GetRow(board, 0).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 1).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 2).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 3).m_data; \
		hash *= 16777619; \
	} \
	auto t2 = std::chrono::high_resolution_clock::now(); \
	uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); \
	std::cout << "    time=" << CPU_FREQ_MHZ * time / (loops / 1000) << " mcy, hash=" << hash << std::endl; \
}

void Test_NormalizeBoard() {
	std::cout << "**** Test_NormalizeBoard ****" << std::endl;
	std::cout << "Reference:" << std::endl;
	TEST_NORMALIZE_BOARD(board);
	TEST_NORMALIZE_BOARD(board);
	std::cout << "Normalize:" << std::endl;
	TEST_NORMALIZE_BOARD(Normalize(board));
	TEST_NORMALIZE_BOARD(Normalize(board));
	std::cout << "Normalize+FlipLR:" << std::endl;
	TEST_NORMALIZE_BOARD(Normalize(FlipLR(board)));
	TEST_NORMALIZE_BOARD(Normalize(FlipLR(board)));
	std::cout << "Normalize+FlipUD:" << std::endl;
	TEST_NORMALIZE_BOARD(Normalize(FlipUD(board)));
	TEST_NORMALIZE_BOARD(Normalize(FlipUD(board)));
	std::cout << "Normalize+Transpose:" << std::endl;
	TEST_NORMALIZE_BOARD(Normalize(Transpose(board)));
	TEST_NORMALIZE_BOARD(Normalize(Transpose(board)));
	std::cout << "----------------------------------------" << std::endl;
}

#define TEST_HASH_BOARD(CODE) { \
	unsigned int hash = 2166136261; \
	std::mt19937_64 rng(12345); \
	auto t1 = std::chrono::high_resolution_clock::now(); \
	unsigned int loops = 10000000; \
	for(unsigned int i = 0; i < loops; ++i) { \
		Board board{rng()}; \
		board = Board{CODE}; \
		hash ^= GetRow(board, 0).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 1).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 2).m_data; \
		hash *= 16777619; \
		hash ^= GetRow(board, 3).m_data; \
		hash *= 16777619; \
	} \
	auto t2 = std::chrono::high_resolution_clock::now(); \
	uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); \
	std::cout << "    time=" << CPU_FREQ_MHZ * time / (loops / 1000) << " mcy, hash=" << hash << std::endl; \
}

void Test_HashBoard() {
	std::cout << "**** Test_HashBoard ****" << std::endl;
	std::cout << "Hash1:" << std::endl;
	TEST_HASH_BOARD(Hash1(board));
	TEST_HASH_BOARD(Hash1(board));
	std::cout << "Hash2:" << std::endl;
	TEST_HASH_BOARD(Hash2(board));
	TEST_HASH_BOARD(Hash2(board));
	std::cout << "Hash3:" << std::endl;
	TEST_HASH_BOARD(Hash3(board));
	TEST_HASH_BOARD(Hash3(board));
	std::cout << "----------------------------------------" << std::endl;
}
