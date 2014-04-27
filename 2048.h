/*
 * Copyright (c) 2014 Maarten Baert <maarten-baert@hotmail.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <stdint.h>

#define FIELD_SIZE 4

enum enum_direction {
	DIRECTION_UP    = 0,
	DIRECTION_DOWN  = 1,
	DIRECTION_LEFT  = 2,
	DIRECTION_RIGHT = 3,
};

typedef uint8_t cell_t;

struct Field {
	cell_t cells[FIELD_SIZE][FIELD_SIZE];
	inline bool operator==(const Field& other) const {
		for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
				if(cells[i][j] != other.cells[i][j])
					return false;
			}
		}
		return true;
	}
	inline bool operator!=(const Field& other) const {
		return !(*this == other);
	}
};

void ClearField(Field* out);
void PrintField(const Field& in);
unsigned int CountFreeCells(const Field& in);

void NormalizeField(Field* out, const Field& in);

bool ApplyGravity(Field* out, const Field& in, enum_direction direction, unsigned int *score);
bool InsertBlock(Field* out, const Field& in, unsigned int location, unsigned int value);

