#include "Minimax.h"

#include <cassert>
#include <ctime>

#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

#define MAX_VALUE 15 // 2^15 = 32768

const unsigned int PARAMETERS_MIN[PARAM_COUNT] = {0};
const unsigned int PARAMETERS_MAX[PARAM_COUNT] = {
	1000000,
	1000000,
	1000,
	1000,
	1000000,
	1000,
};
const unsigned int PARAMETERS_STEP[PARAM_COUNT] = {
	2000,
	100,
	50,
	50,
	50,
	5,
};

void GetDefaultHeuristicParameters(HeuristicParameters* parameters) {

	// the original:
	/*parameters->m_score_stillalive = 100000;
	parameters->m_score_freecell = 10000;
	parameters->m_score_centerofmass = 10;*/

	// used by half4:
	/*parameters->m_score_stillalive = 23000;
	parameters->m_score_freecell = 3000;
	parameters->m_score_centerofmass = 2;*/

	// used by tune1 (DO NOT CHANGE):
	/*parameters->m_score_stillalive = 13000;
	parameters->m_score_freecell1 = 800;
	parameters->m_score_freecell2 = 0;
	parameters->m_score_centerofmass = 120;*/

	// next:
	/*parameters->m_score_stillalive = 9000;
	parameters->m_score_freecell1 = 600;
	parameters->m_score_freecell2 = 0;
	parameters->m_score_centerofmass = 110;*/

	// tuning at depth 4/5
	parameters->m_values[PARAM_STILLALIVE] = 4800;     // depth 3: 3300
	parameters->m_values[PARAM_FREECELL] = 220;        // depth 3: 80
	parameters->m_values[PARAM_CENTEROFMASS] = 110;    // depth 3: 160
	parameters->m_values[PARAM_CENTEROFMASS2] = 130;   // depth 3: 190
	parameters->m_values[PARAM_USEFUL] = 170;          // depth 3: 90
	parameters->m_values[PARAM_USEFUL_LOOKAHEAD] = 18; // depth 3: 21

}

namespace std {
template<>
struct hash<Field> {
	inline size_t operator()(const Field& field) const {
		// FNV-1a hash
		//uint64_t h = 14695981039346656037ull;
		uint32_t h = 2166136261;
		for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
				//h = (h ^ field.cells[i][j]) * 1099511628211ull;
				h = (h ^ field.cells[i][j]) * 16777619;
			}
		}
		return h;
	}
};
}

struct CachedResult {
	unsigned int m_score;
};

struct MinimaxHelpers {
	HeuristicParameters m_parameters;
	std::mt19937 m_rng;
	std::vector<std::unordered_map<Field, CachedResult> > m_cache_computer;
	unsigned int m_cache_computer_hit, m_cache_computer_miss;
	inline MinimaxHelpers(const HeuristicParameters& parameters, unsigned int moves) {
		m_parameters = parameters;
		m_rng.seed(clock());
		m_cache_computer.resize(moves);
		m_cache_computer_hit = 0;
		m_cache_computer_miss = 0;
	}
};

inline __attribute__((always_inline))
unsigned int HeuristicScore(const Field& field, MinimaxHelpers* helpers) {
	unsigned int score = helpers->m_parameters.m_values[PARAM_STILLALIVE], freecell = helpers->m_parameters.m_values[PARAM_FREECELL];
	int ci = 0, cj = 0;
	int ci2 = 0, cj2 = 0;
	unsigned int histogram[MAX_VALUE] = {0};
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			uint8_t value = field.cells[i][j];
			if(value == 0) {
				score += freecell;
				freecell >>= 1;
			} else {
				ci += ((int) i * 2 - (FIELD_SIZE - 1)) * value * value;
				cj += ((int) j * 2 - (FIELD_SIZE - 1)) * value * value;
				ci2 += ((int) i * 2 - (FIELD_SIZE - 1)) * (1 << value);
				cj2 += ((int) j * 2 - (FIELD_SIZE - 1)) * (1 << value);
				++histogram[value - 1];
			}
		}
	}
	score += ((abs(ci) + abs(cj)) * helpers->m_parameters.m_values[PARAM_CENTEROFMASS]) >> 8;
	score += ((abs(ci2) + abs(cj2)) * helpers->m_parameters.m_values[PARAM_CENTEROFMASS2]) >> 8;
	{
		bool useful[MAX_VALUE + 1];
		useful[0] = true;
		unsigned int total_value = helpers->m_parameters.m_values[PARAM_USEFUL_LOOKAHEAD];
		for(unsigned int i = 0; i < MAX_VALUE; ++i) {
			total_value += histogram[i] * (1 << i);
			useful[i + 1] = (total_value >= (2u << i) + i);
		}
		for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE - 1; ++j) {
				uint8_t value1 = field.cells[i][j];
				uint8_t value2 = field.cells[i][j + 1];
				if(useful[value1] == useful[value2])
					score += helpers->m_parameters.m_values[PARAM_USEFUL];
			}
		}
		for(unsigned int i = 0; i < FIELD_SIZE - 1; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
				uint8_t value1 = field.cells[i][j];
				uint8_t value2 = field.cells[i + 1][j];
				if(useful[value1] == useful[value2])
					score += helpers->m_parameters.m_values[PARAM_USEFUL];
			}
		}
	}
	return score;
}

unsigned int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers);
unsigned int MinimaxTurnComputer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers);
unsigned int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers);

inline __attribute__((always_inline))
void TurnComputer(const Field& field, unsigned int moves_left, unsigned int& score_sum, unsigned int& score_div, MinimaxHelpers* helpers, unsigned int location, unsigned int value) {
	Field newfield;
	if(InsertBlock(&newfield, field, location, value)) {
		score_sum += MinimaxTurnPlayer(newfield, moves_left, helpers);
		++score_div;
	}
}

inline __attribute__((always_inline))
bool TurnPlayer(const Field& field, unsigned int moves_left, unsigned int& best_score, MinimaxHelpers* helpers, unsigned int direction) {
	Field newfield;
	unsigned int score = 0;
	if(ApplyGravity(&newfield, field, (enum_direction) direction, &score)) {
		score += CachedMinimaxTurnComputer(newfield, moves_left - 1, helpers);
		if(score > best_score) {
			best_score = score;
			return true;
		}
	}
	return false;
}

unsigned int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers) {

	if(moves_left == 0)
		return HeuristicScore(field, helpers);

	// normalize the field
	Field normfield;
	NormalizeField(&normfield, field);

	// check the cache
	CachedResult dummy;
	auto cache = helpers->m_cache_computer[moves_left].emplace(normfield, dummy);
	CachedResult &cache_result = cache.first->second;
	if(cache.second) {
		++helpers->m_cache_computer_miss;
		unsigned int score = MinimaxTurnComputer(normfield, moves_left, helpers);
		cache_result.m_score = score;
		return score;
	} else {
		++helpers->m_cache_computer_hit;
		return cache_result.m_score;
	}

}

//unsigned int g_todo_table[20] = {2, 3, 4, 6, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};

unsigned int MinimaxTurnComputer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers) {
	assert(moves_left != 0);
	unsigned int locations[FIELD_SIZE * FIELD_SIZE], location_count = 0;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			if(field.cells[i][j] == 0) {
				locations[location_count] = i * FIELD_SIZE + j;
				++location_count;
			}
		}
	}
	//assert(moves_left <= 20);
	//unsigned int todo = std::min(g_todo_table[moves_left - 1], location_count);
	unsigned int todo = std::min(1u << moves_left, location_count);
	if(location_count > 2) {
		unsigned int score_sum = 0, score_div = 0;
		for(unsigned int k = 0; k < todo; ++k) {
			unsigned int p = helpers->m_rng() % location_count;
			unsigned int location = locations[p];
			TurnComputer(field, moves_left, score_sum, score_div, helpers, location, (helpers->m_rng() % 10 == 0)? 2 : 1);
			--location_count;
			locations[p] = locations[location_count];
		}
		assert(score_div != 0);
		return (score_sum + score_div / 2) / score_div;
	} else {
		unsigned int score1_sum = 0, score1_div = 0;
		unsigned int score2_sum = 0, score2_div = 0;
		for(unsigned int k = 0; k < todo; ++k) {
			unsigned int p = helpers->m_rng() % location_count;
			unsigned int location = locations[p];
			TurnComputer(field, moves_left, score1_sum, score1_div, helpers, location, 1);
			TurnComputer(field, moves_left, score2_sum, score2_div, helpers, location, 2);
			--location_count;
			locations[p] = locations[location_count];
		}
		unsigned int score_sum = score1_sum * 9 + score2_sum, score_div = score1_div * 9 + score2_div;
		assert(score_div != 0);
		return (score_sum + score_div / 2) / score_div;
	}
}

unsigned int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, MinimaxHelpers* helpers) {
	assert(moves_left != 0);
	unsigned int best_score = 0;
	for(unsigned int direction = 0; direction < 4; ++direction) {
		TurnPlayer(field, moves_left, best_score, helpers, direction);
	}
	return best_score;
}

unsigned int MinimaxBestMove(const Field& field, unsigned int moves, const HeuristicParameters& parameters) {
	/*if(CountFreeCells(field) < parameters.m_values[PARAM_EXTENDMOVES]) {
		++moves;
	}*/
	MinimaxHelpers helpers(parameters, moves);
	unsigned int best_score = 0, best_move = (unsigned int) -1;
	for(unsigned int direction = 0; direction < 4; ++direction) {
		Field newfield;
		unsigned int score = 0;
		if(ApplyGravity(&newfield, field, (enum_direction) direction, &score)) {
			score += CachedMinimaxTurnComputer(newfield, moves - 1, &helpers);
			if(score > best_score || best_move == (unsigned int) -1) {
				best_score = score;
				best_move = direction;
			}
		}
	}
	return best_move;
}
