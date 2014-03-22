#pragma once
#include "2048.h"

struct HeuristicParameters {
	unsigned int m_score_stillalive;
	unsigned int m_score_freecell;
	unsigned int m_score_centerofmass;
};

void GetDefaultHeuristicParameters(HeuristicParameters* parameters);

unsigned int MinimaxBestScore(unsigned int min_moves, unsigned int max_moves, unsigned int step, const HeuristicParameters& parameters);
unsigned int MinimaxBestMove(const Field& field, unsigned int moves, const HeuristicParameters& parameters);
