#pragma once
#include "2048.h"

enum enum_parameters {
	PARAM_STILLALIVE,
	PARAM_FREECELL,
	PARAM_CENTEROFMASS,
	PARAM_CENTEROFMASS2,
	PARAM_USEFUL,
	PARAM_USEFUL_LOOKAHEAD,
//	PARAM_USEFUL_ENABLE,
	PARAM_COUNT
};

struct HeuristicParameters {
	unsigned int m_values[PARAM_COUNT];
};

extern const unsigned int PARAMETERS_MIN[PARAM_COUNT];
extern const unsigned int PARAMETERS_MAX[PARAM_COUNT];
extern const unsigned int PARAMETERS_STEP[PARAM_COUNT];

void GetDefaultHeuristicParameters(HeuristicParameters* parameters);

unsigned int MinimaxBestMove(const Field& field, unsigned int moves, const HeuristicParameters& parameters);
