#include "2048.h"

#include <cassert>
#include <cstring>

#include <iostream>

void ClearField(Field* out) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			out->cells[i][j] = 0;
		}
	}
}

void PrintField(const Field& in) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		std::cout << "[";
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			uint8_t value = in.cells[i][j];
			std::cout.width(5);
			std::cout  << std::right << ((value == 0)? 0 : (1 << value));
		}
		std::cout << " ]" << std::endl;
	}
	std::cout << std::endl;
}

unsigned int CountFreeCells(const Field& in) {
	unsigned int count = 0;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			if(in.cells[i][j] == 0)
				++count;
		}
	}
	return count;
}

void NormalizeField(Field* out, const Field& in) {
	int n = FIELD_SIZE - 1;
	int matrices[8][6] = {
		{0, +1, 0, 0, 0, +1},
		{0, +1, 0, n, 0, -1},
		{n, -1, 0, 0, 0, +1},
		{n, -1, 0, n, 0, -1},
		{0, 0, +1, 0, +1, 0},
		{0, 0, +1, n, -1, 0},
		{n, 0, -1, 0, +1, 0},
		{n, 0, -1, n, -1, 0},
	};
	int indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	unsigned int remaining = 8;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			int values[8], highest = -1;
			for(unsigned int k = 0; k < remaining; ++k) {
				int *matrix = matrices[indices[k]];
				values[k] = in.cells[matrix[0] + matrix[1] * i + matrix[2] * j][matrix[3] + matrix[4] * i + matrix[5] * j];
				if(values[k] > highest)
					highest = values[k];
			}
			for(unsigned int k = remaining; k > 0; ) {
				--k;
				if(values[k] != highest) {
					--remaining;
					indices[k] = indices[remaining];
				}
			}
			if(remaining == 1)
				goto done;
		}
	}
	assert(remaining > 1);
	done:
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			int *matrix = matrices[indices[0]];
			out->cells[i][j] = in.cells[matrix[0] + matrix[1] * i + matrix[2] * j][matrix[3] + matrix[4] * i + matrix[5] * j];
		}
	}
}

inline __attribute__((always_inline))
bool CanMoveRow(const cell_t* row_in, int stride) {
	for(unsigned int i = 0; i < FIELD_SIZE - 1; ++i) {
		unsigned int in1 = *row_in;
		row_in += stride;
		unsigned int in2 = *row_in;
		if(in2 != 0 && (in1 == 0 || in1 == in2))
			return true;
	}
	return false;
}

inline __attribute__((always_inline))
void MoveRow(cell_t* row_out, const cell_t* row_in, int stride, int* score) {
	unsigned int out = 0;
	cell_t *row_out_end = row_out + stride * FIELD_SIZE;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		unsigned int in = *row_in;
		if(in != 0) {
			if(out == 0) {
				out = in;
			} else if(out == in) {
				*score += 1 << (out + 1);
				*row_out = out + 1;
				row_out += stride;
				out = 0;
			} else {
				*row_out = out;
				row_out += stride;
				out = in;
			}
		}
		row_in += stride;
	}
	*row_out = out;
	row_out += stride;
	while(row_out != row_out_end) {
		*row_out = 0;
		row_out += stride;
	}
}

inline __attribute__((always_inline))
bool ApplyGravityUp(Field* out, const Field& in, int* score) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		if(CanMoveRow(&(in.cells[0][i]), FIELD_SIZE)) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j)
				MoveRow(&(out->cells[0][j]), &(in.cells[0][j]), FIELD_SIZE, score);
			assert(in != *out);
			return true;
		}
	}
	return false;
}

inline __attribute__((always_inline))
bool ApplyGravityDown(Field* out, const Field& in, int* score) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		if(CanMoveRow(&(in.cells[FIELD_SIZE - 1][i]), -FIELD_SIZE)) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j)
				MoveRow(&(out->cells[FIELD_SIZE - 1][j]), &(in.cells[FIELD_SIZE - 1][j]), -FIELD_SIZE, score);
			assert(in != *out);
			return true;
		}
	}
	return false;
}

inline __attribute__((always_inline))
bool ApplyGravityLeft(Field* out, const Field& in, int* score) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		if(CanMoveRow(&(in.cells[i][0]), 1)) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j)
				MoveRow(&(out->cells[j][0]), &(in.cells[j][0]), 1, score);
			assert(in != *out);
			return true;
		}
	}
	return false;
}

inline __attribute__((always_inline))
bool ApplyGravityRight(Field* out, const Field& in, int* score) {
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		if(CanMoveRow(&(in.cells[i][FIELD_SIZE - 1]), -1)) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j)
				MoveRow(&(out->cells[j][FIELD_SIZE - 1]), &(in.cells[j][FIELD_SIZE - 1]), -1, score);
			assert(in != *out);
			return true;
		}
	}
	return false;
}

bool ApplyGravity(Field* out, const Field& in, enum_direction direction, int* score) {
	switch(direction) {
		case DIRECTION_UP: return ApplyGravityUp(out, in, score);
		case DIRECTION_DOWN: return ApplyGravityDown(out, in, score);
		case DIRECTION_LEFT: return ApplyGravityLeft(out, in, score);
		case DIRECTION_RIGHT: return ApplyGravityRight(out, in, score);
		default: assert(false); return false;
	}
}

bool InsertBlock(Field* out, const Field& in, unsigned int location, unsigned int value) {
	unsigned int i = location / FIELD_SIZE, j = location % FIELD_SIZE;
	if(in.cells[i][j] == 0) {
		*out = in;
		out->cells[i][j] = value;
		return true;
	}
	return false;
}
