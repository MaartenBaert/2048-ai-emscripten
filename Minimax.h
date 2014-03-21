#pragma once
#include "2048.h"

int MinimaxBestScore(unsigned int min_moves, unsigned int max_moves, unsigned int step);
unsigned int MinimaxBestMove(const Field& field, unsigned int max_moves, bool computer_starts);
