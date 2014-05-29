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

#include "ExpectiMax.h"

// it's hard to pass JS arrays to C++, this is simpler ;)
extern "C"
unsigned int JS_MinimaxBestMove(unsigned int f00, unsigned int f01, unsigned int f02, unsigned int f03,
								unsigned int f10, unsigned int f11, unsigned int f12, unsigned int f13,
								unsigned int f20, unsigned int f21, unsigned int f22, unsigned int f23,
								unsigned int f30, unsigned int f31, unsigned int f32, unsigned int f33) {
	Board board = MakeBoard(
		MakeRow(f00, f01, f02, f03),
		MakeRow(f10, f11, f12, f13),
		MakeRow(f20, f21, f22, f23),
		MakeRow(f30, f31, f32, f33)
	);
	HeuristicParameters parameters;
	GetDefaultHeuristicParameters(&parameters);
	auto p = FindBestMove(board, parameters);
	return p.first;
}
