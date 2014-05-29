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

#include "Board.h"

const LookupTable g_board_table;

LookupTable::LookupTable() {
	for(unsigned int i = 0; i < 0x10000; ++i) {
		BoardRow row{(uint16_t) i};
		BoardRowScore result = SlowCollapseLeft(row);
		m_table_collapse_left[row.m_data] = result;
		m_table_collapse_right[Reverse(row).m_data] = BoardRowScore{Reverse(result.m_row), result.m_score};
	}
}

BoardRowScore SlowCollapseLeft(BoardRow row) {
	BoardRowScore result{0, 0};
	unsigned int hold = 0, pos = 0;
	for(unsigned int i = 0; i < 4; ++i) {
		unsigned int next = GetCell(row, i);
		if(next != 0) {
			if(hold == 0) {
				hold = next;
			} else if(hold == next) {
				result.m_row = SetCell(result.m_row, pos, hold + 1);
				result.m_score += 1 << hold;
				++pos;
				hold = 0;
			} else {
				result.m_row = SetCell(result.m_row, pos, hold);
				++pos;
				hold = next;
			}
		}
	}
	result.m_row = SetCell(result.m_row, pos, hold);
	return result;
}
