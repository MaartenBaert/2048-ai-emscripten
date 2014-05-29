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

#include "Board.h"

enum enum_parameters {
	PARAM_STILLALIVE,
	PARAM_FREECELL,
	PARAM_CENTEROFMASS1,
	PARAM_CENTEROFMASS2,
	PARAM_CENTEROFMASS3,
	PARAM_CENTEROFMASS4,
	PARAM_COUNT
};

struct HeuristicParameters {
	unsigned int m_values[PARAM_COUNT];
};

extern const unsigned int PARAMETERS_MIN[PARAM_COUNT];
extern const unsigned int PARAMETERS_MAX[PARAM_COUNT];
extern const unsigned int PARAMETERS_STEP[PARAM_COUNT];

void GetDefaultHeuristicParameters(HeuristicParameters* parameters);
void PrintExpectiMaxStats();
std::pair<enum_direction, unsigned int> FindBestMove(Board board, const HeuristicParameters& parameters);
