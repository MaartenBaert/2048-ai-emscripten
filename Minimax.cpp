#include "Minimax.h"

#include <cassert>
#include <ctime>

#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

void GetDefaultHeuristicParameters(HeuristicParameters* parameters) {

	// the original:
	/*parameters->m_score_stillalive = 100000;
	parameters->m_score_freecell = 10000;
	parameters->m_score_centerofmass = 10;*/

	// used by half4:
	/*parameters->m_score_stillalive = 23000;
	parameters->m_score_freecell = 3000;
	parameters->m_score_centerofmass = 2;*/

	// used by sq2
	parameters->m_score_stillalive = 18000;
	parameters->m_score_freecell = 2400;
	parameters->m_score_centerofmass = 197;

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

unsigned int HeuristicScore(const Field& field, MinimaxHelpers* helpers) {
	unsigned int score = helpers->m_parameters.m_score_stillalive;
	int ci = 0, cj = 0;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			uint8_t value = field.cells[i][j];
			if(value == 0) {
				score += helpers->m_parameters.m_score_freecell;
			} else {
				ci += ((int) i * 2 - (FIELD_SIZE - 1)) * value * value;
				cj += ((int) j * 2 - (FIELD_SIZE - 1)) * value * value;
			}
		}
	}
	score += ((abs(ci) + abs(cj)) * helpers->m_parameters.m_score_centerofmass) >> 8;
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

	// maximum depth reached?
	if(moves_left == 0)
		return HeuristicScore(field, helpers);

	// normalize the field
	Field normfield;
	NormalizeField(&normfield, field);

	// check the cache
	CachedResult dummy;
	auto cache = helpers->m_cache_computer[moves_left - 1].emplace(normfield, dummy);
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
	unsigned int todo = std::min(1u << moves_left, location_count);
	if(location_count > 3) {
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

unsigned int MinimaxBestScore(unsigned int min_moves, unsigned int max_moves, unsigned int step, const HeuristicParameters& parameters) {

	unsigned int score = 0;
	for(unsigned int moves = min_moves; moves <= max_moves; moves += step) {

		// initialize
		MinimaxHelpers helpers(parameters, moves);
		Field field;
		ClearField(&field);

		// run minimax
		score = CachedMinimaxTurnComputer(field, moves, &helpers);

		// print stats
		std::cout << "Moves: " << moves << ", Best score: " << score << std::endl;
		std::cout << "**** Cache computer - hit=" << helpers.m_cache_computer_hit
				  << " miss=" << helpers.m_cache_computer_miss
				  << std::endl;

	}

	return score;
}

unsigned int MinimaxBestMove(const Field& field, unsigned int moves, const HeuristicParameters& parameters) {
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
