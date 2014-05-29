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

#include <cassert>
#include <stdint.h>

#include <algorithm>

// basic types
enum enum_direction {
	DIRECTION_NONE = -1,
	DIRECTION_LEFT  = 0,
	DIRECTION_RIGHT = 1,
	DIRECTION_UP    = 2,
	DIRECTION_DOWN  = 3,
};
struct BoardRow {
	uint16_t m_data; // 4 bits per cell
};
struct Board {
	uint64_t m_data; // 4 bits per cell, 16 bits per row
};
struct BoardRowScore {
	BoardRow m_row;
	uint16_t m_score; // real score divided by two (since it is always even)
};
struct BoardScore {
	Board m_board;
	unsigned int m_score; // real score divided by two (since it is always even)
};

// lookup table
struct LookupTable {

	BoardRowScore m_table_collapse_left[0x10000];
	BoardRowScore m_table_collapse_right[0x10000];

	LookupTable();

};
extern const LookupTable g_board_table;

BoardRowScore SlowCollapseLeft(BoardRow row);

// row manipulation
inline BoardRow MakeRow(unsigned int v0, unsigned int v1, unsigned int v2, unsigned int v3) {
	return BoardRow{(uint16_t) (v0 | (v1 << 4) | (v2 << 8) | (v3 << 12))};
}
inline unsigned int GetCell(BoardRow row, unsigned int i) {
	return (row.m_data >> (i * 4)) & 0xf;
}
inline BoardRow SetCell(BoardRow row, unsigned int i, unsigned int value) {
	return BoardRow{(uint16_t) ((row.m_data & ~(0xf << (i * 4))) | (value << (i * 4)))};
}

inline BoardRow Reverse(BoardRow row) {
	return BoardRow{(uint16_t) (((row.m_data & 0x000f) << 12) |
								((row.m_data & 0x00f0) << 4) |
								((row.m_data & 0x0f00) >> 4) |
								((row.m_data & 0xf000) >> 12))};
}

inline BoardRowScore CollapseLeft(BoardRow row) {
	return g_board_table.m_table_collapse_left[row.m_data];
}
inline BoardRowScore CollapseRight(BoardRow row) {
	return g_board_table.m_table_collapse_right[row.m_data];
}
inline BoardRowScore CollapseRight2(BoardRow row) {
	BoardRowScore result = g_board_table.m_table_collapse_left[Reverse(row).m_data];
	return BoardRowScore{Reverse(result.m_row), result.m_score};
}

// board manipulation
inline Board MakeBoard(BoardRow r0, BoardRow r1, BoardRow r2, BoardRow r3) {
	return Board{((uint64_t) r0.m_data) | ((uint64_t) r1.m_data << 16) | ((uint64_t) r2.m_data << 32) | ((uint64_t) r3.m_data << 48)};
}
inline BoardRow GetRow(Board board, unsigned int j) {
	return BoardRow{(uint16_t) ((board.m_data >> (j * 16)) & 0xffff)};
}
inline Board SetRow(Board board, unsigned int j, BoardRow value) {
	return Board{(board.m_data & ~((uint64_t) 0xffff << (j * 16))) | ((uint64_t) value.m_data << (j * 16))};
}
inline unsigned int GetCell(Board board, unsigned int ij) {
	return (board.m_data >> (ij * 4)) & 0xf;
}
inline unsigned int GetCell(Board board, unsigned int i, unsigned int j) {
	return (board.m_data >> (i * 4 + j * 16)) & 0xf;
}
inline Board SetCell(Board board, unsigned int ij, unsigned int value) {
	return Board{(board.m_data & ~((uint64_t) 0xf << (ij * 4))) | ((uint64_t) value << (ij * 4))};
}
inline Board SetCell(Board board, unsigned int i, unsigned int j, unsigned int value) {
	return Board{(board.m_data & ~((uint64_t) 0xf << (i * 4 + j * 16))) | ((uint64_t) value << (i * 4 + j * 16))};
}

inline Board FlipLR(Board board) {
	return Board{((board.m_data & UINT64_C(0x000f000f000f000f)) << 12) |
				 ((board.m_data & UINT64_C(0x00f000f000f000f0)) << 4) |
				 ((board.m_data & UINT64_C(0x0f000f000f000f00)) >> 4) |
				 ((board.m_data & UINT64_C(0xf000f000f000f000)) >> 12)};
}
inline Board FlipUD(Board board) {
	return Board{((board.m_data & UINT64_C(0x000000000000ffff)) << 48) |
				 ((board.m_data & UINT64_C(0x00000000ffff0000)) << 16) |
				 ((board.m_data & UINT64_C(0x0000ffff00000000)) >> 16) |
				 ((board.m_data & UINT64_C(0xffff000000000000)) >> 48)};
}
inline Board Transpose(Board board) {
	return Board{((board.m_data & UINT64_C(0x000000000000f000)) << 36) |
				 ((board.m_data & UINT64_C(0x000f000000000000)) >> 36) |
				 ((board.m_data & UINT64_C(0x00000000f0000f00)) << 24) |
				 ((board.m_data & UINT64_C(0x00f0000f00000000)) >> 24) |
				 ((board.m_data & UINT64_C(0x0000f0000f0000f0)) << 12) |
				 ((board.m_data & UINT64_C(0x0f0000f0000f0000)) >> 12) |
				 ((board.m_data & UINT64_C(0xf0000f0000f0000f)))};
}

inline BoardScore CollapseLeft(Board board) {
	BoardRowScore result0 = CollapseLeft(GetRow(board, 0));
	BoardRowScore result1 = CollapseLeft(GetRow(board, 1));
	BoardRowScore result2 = CollapseLeft(GetRow(board, 2));
	BoardRowScore result3 = CollapseLeft(GetRow(board, 3));
	return BoardScore{MakeBoard(result0.m_row, result1.m_row, result2.m_row, result3.m_row),
					  (unsigned int) (result0.m_score + result1.m_score + result2.m_score + result3.m_score)};
}
inline BoardScore CollapseRight(Board board) {
	BoardRowScore result0 = CollapseRight(GetRow(board, 0));
	BoardRowScore result1 = CollapseRight(GetRow(board, 1));
	BoardRowScore result2 = CollapseRight(GetRow(board, 2));
	BoardRowScore result3 = CollapseRight(GetRow(board, 3));
	return BoardScore{MakeBoard(result0.m_row, result1.m_row, result2.m_row, result3.m_row),
					  (unsigned int) (result0.m_score + result1.m_score + result2.m_score + result3.m_score)};
}
inline BoardScore CollapseUp(Board board) {
	board = Transpose(board);
	BoardRowScore result0 = CollapseLeft(GetRow(board, 0));
	BoardRowScore result1 = CollapseLeft(GetRow(board, 1));
	BoardRowScore result2 = CollapseLeft(GetRow(board, 2));
	BoardRowScore result3 = CollapseLeft(GetRow(board, 3));
	return BoardScore{Transpose(MakeBoard(result0.m_row, result1.m_row, result2.m_row, result3.m_row)),
					  (unsigned int) (result0.m_score + result1.m_score + result2.m_score + result3.m_score)};
}
inline BoardScore CollapseDown(Board board) {
	board = Transpose(board);
	BoardRowScore result0 = CollapseRight(GetRow(board, 0));
	BoardRowScore result1 = CollapseRight(GetRow(board, 1));
	BoardRowScore result2 = CollapseRight(GetRow(board, 2));
	BoardRowScore result3 = CollapseRight(GetRow(board, 3));
	return BoardScore{Transpose(MakeBoard(result0.m_row, result1.m_row, result2.m_row, result3.m_row)),
					  (unsigned int) (result0.m_score + result1.m_score + result2.m_score + result3.m_score)};
}
inline BoardScore Collapse(Board board, enum_direction direction) {
	switch(direction) {
		case DIRECTION_LEFT: return CollapseLeft(board);
		case DIRECTION_RIGHT: return CollapseRight(board);
		case DIRECTION_UP: return CollapseUp(board);
		case DIRECTION_DOWN: return CollapseDown(board);
		default: assert(false); return BoardScore{0, 0};
	}
}

inline Board Normalize(Board board) {
	Board b000 = board;
	Board b001 = Transpose(b000);
	Board b010 = FlipLR(b000), b011 = FlipLR(b001);
	Board b100 = FlipUD(b000), b101 = FlipUD(b001), b110 = FlipUD(b010), b111 = FlipUD(b011);
	return Board{std::max(std::max(std::max(b000.m_data, b001.m_data), std::max(b010.m_data, b011.m_data)),
						  std::max(std::max(b100.m_data, b101.m_data), std::max(b110.m_data, b111.m_data)))};
}
inline uint32_t Hash1(Board board) {
	// 32-bit FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/index.html)
	// this variant hashes two bytes at once, it is faster but not as good
	uint32_t h = 2166136261;
	h ^= (board.m_data      ) & 0xffff; h *= 16777619;
	h ^= (board.m_data >> 16) & 0xffff; h *= 16777619;
	h ^= (board.m_data >> 32) & 0xffff; h *= 16777619;
	h ^= (board.m_data >> 48) & 0xffff; h *= 16777619;
	return h ^ (h >> 16);
}
inline uint32_t Hash2(Board board) {
	// 64-bit FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/index.html)
	// this variant hashes two bytes at once, it is faster but not as good
	uint64_t h = UINT64_C(14695981039346656037);
	h ^= (board.m_data      ) & 0xffff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 16) & 0xffff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 32) & 0xffff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 48) & 0xffff; h *= UINT64_C(1099511628211);
	return (uint32_t) h ^ (uint32_t) (h >> 32);
}
inline uint32_t Hash3(Board board) {
	// 64-bit FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/index.html)
	uint64_t h = UINT64_C(14695981039346656037);
	h ^= (board.m_data      ) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >>  8) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 16) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 24) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 32) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 40) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 48) & 0xff; h *= UINT64_C(1099511628211);
	h ^= (board.m_data >> 56) & 0xff; h *= UINT64_C(1099511628211);
	return (uint32_t) h ^ (uint32_t) (h >> 32);
}
