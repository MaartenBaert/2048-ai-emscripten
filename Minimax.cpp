#include "Minimax.h"

#include <cassert>

#include <iostream>
#include <limits>
#include <vector>
#include <unordered_map>

#define SCORE_MIN        -1000000000
#define SCORE_MAX         1000000000
#define SCORE_STILLALIVE   100000000
#define SCORE_FREECELL       1000000
#define SCORE_CENTEROFMASS        10
//#define SCORE_PAIR_EQUAL           2
//#define SCORE_PAIR_CLOSE           1

struct CachedResult {
	int score_lo, score_hi;
};

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

struct MinimaxHelpers {
	std::vector<unsigned int> m_killermove_computer;
	std::vector<unsigned int> m_killermove_player;
	std::vector<std::unordered_map<Field, CachedResult> > m_cache_computer;
	/*unsigned int m_killermove_computer_hit, m_killermove_computer_miss;*/
	unsigned int m_killermove_player_hit, m_killermove_player_miss;
	unsigned int m_cache_computer_hit, m_cache_computer_partial, m_cache_computer_miss;
	unsigned int m_dead_ends;
	inline MinimaxHelpers(unsigned int moves) {
		m_killermove_computer.resize(moves, 0);
		m_killermove_player.resize(moves, 0);
		m_cache_computer.resize(moves);
		/*m_killermove_computer_hit = 0;
		m_killermove_computer_miss = 0;*/
		m_killermove_player_hit = 0;
		m_killermove_player_miss = 0;
		m_cache_computer_hit = 0;
		m_cache_computer_partial = 0;
		m_cache_computer_miss = 0;
		m_dead_ends = 0;
	}
};

int HeuristicScore(const Field& field) {
	int score = SCORE_STILLALIVE;
	int ci = 0, cj = 0;
	for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			uint8_t value = field.cells[i][j];
			if(value == 0) {
				score += SCORE_FREECELL;
			} else {
				ci += ((int) i * 2 - (FIELD_SIZE - 1)) * value;
				cj += ((int) j * 2 - (FIELD_SIZE - 1)) * value;
			}
		}
	}
	score += (abs(ci) + abs(cj)) * SCORE_CENTEROFMASS;
	/*for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE - 1; ++j) {
			uint8_t value1 = field.cells[i][j];
			uint8_t value2 = field.cells[i][j + 1];
			if(value1 != 0 && value2 != 0) {
				if(value1 == value2)
					score += (1 << value1) * SCORE_PAIR_EQUAL;
				if(value1 == value2 + 1)
					score += (1 << value1) * SCORE_PAIR_CLOSE;
				if(value1 + 1 == value2)
					score += (1 << value2) * SCORE_PAIR_CLOSE;
			}
		}
	}
	for(unsigned int i = 0; i < FIELD_SIZE - 1; ++i) {
		for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
			uint8_t value1 = field.cells[i][j];
			uint8_t value2 = field.cells[i + 1][j];
			if(value1 != 0 && value2 != 0) {
				if(value1 == value2)
					score += (1 << value1) * SCORE_PAIR_EQUAL;
				if(value1 == value2 + 1)
					score += (1 << value1) * SCORE_PAIR_CLOSE;
				if(value1 + 1 == value2)
					score += (1 << value2) * SCORE_PAIR_CLOSE;
			}
		}
	}*/
	return score;
}

int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);
int MinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);
int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);

inline __attribute__((always_inline))
bool TurnComputer(const Field& field, unsigned int moves_left, int alpha, int& beta, MinimaxHelpers* helpers, unsigned int location) {
	Field newfield;
	if(InsertBlock(&newfield, field, location, 1)) {
		int score = MinimaxTurnPlayer(newfield, moves_left, alpha, beta, helpers);
		if(score < beta) {
			//helpers->m_killermove_computer[moves_left - 1] = location;
			beta = score;
			if(beta <= alpha)
				return true;
		}
	}
	return false;
}

inline __attribute__((always_inline))
bool TurnPlayer(const Field& field, unsigned int moves_left, int& alpha, int beta, MinimaxHelpers* helpers, unsigned int direction) {
	Field newfield;
	int score = 0;
	if(ApplyGravity(&newfield, field, (enum_direction) direction, &score)) {
		score += CachedMinimaxTurnComputer(newfield, moves_left - 1, alpha - score, beta - score, helpers);
		if(score > alpha) {
			helpers->m_killermove_player[moves_left - 1] = direction;
			alpha = score;
			if(alpha >= beta)
				return true;
		}
	}
	return false;
}

int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {

	// maximum depth reached?
	if(moves_left == 0)
		return HeuristicScore(field);

	// normalize the field
	Field normfield;
	NormalizeField(&normfield, field);

	// check the cache
	CachedResult dummy;
	auto cache = helpers->m_cache_computer[moves_left - 1].emplace(normfield, dummy);
	CachedResult &cache_result = cache.first->second;
	if(cache.second) {
		++helpers->m_cache_computer_miss;

		int result = MinimaxTurnComputer(normfield, moves_left, alpha, beta, helpers);
		if(result <= alpha) {
			cache_result.score_lo = SCORE_MIN;
			cache_result.score_hi = result;
		} else if(result >= beta) {
			cache_result.score_lo = result;
			cache_result.score_hi = SCORE_MAX;
		} else {
			cache_result.score_lo = result;
			cache_result.score_hi = result;
		}

		if(cache_result.score_hi < SCORE_STILLALIVE)
			++helpers->m_dead_ends;

		return result;

	} else {
		++helpers->m_cache_computer_hit;

		if(cache_result.score_hi <= alpha)
			return cache_result.score_hi;
		if(cache_result.score_lo >= beta)
			return cache_result.score_lo;
		if(cache_result.score_lo > alpha && cache_result.score_hi < beta)
			return cache_result.score_lo;

		--helpers->m_cache_computer_hit;
		++helpers->m_cache_computer_partial;

		if(cache_result.score_lo > alpha)
			alpha = cache_result.score_lo;
		if(cache_result.score_hi < beta)
			beta = cache_result.score_hi;

		int result = MinimaxTurnComputer(normfield, moves_left, alpha, beta, helpers);
		assert(result >= cache_result.score_lo);
		assert(result <= cache_result.score_hi);
		if(result <= alpha) {
			if(cache_result.score_hi > result)
				cache_result.score_hi = result;
		} else if(result >= beta) {
			if(cache_result.score_lo < result)
				cache_result.score_lo = result;
		} else {
			if(cache_result.score_lo < result)
				cache_result.score_lo = result;
			if(cache_result.score_hi > result)
				cache_result.score_hi = result;
		}

		if(cache_result.score_hi < SCORE_STILLALIVE)
			++helpers->m_dead_ends;

		return result;

	}

}

int MinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {
	assert(moves_left != 0);
	/*unsigned int killermove = helpers->m_killermove_computer[moves_left - 1];
	if(TurnComputer(field, moves_left, alpha, beta, helpers, killermove)) {
		++helpers->m_killermove_computer_hit;
		return beta;
	}
	++helpers->m_killermove_computer_miss;*/
	for(unsigned int location = 0; location < FIELD_SIZE * FIELD_SIZE; ++location) {
		/*if(location == killermove)
			continue;*/
		if(TurnComputer(field, moves_left, alpha, beta, helpers, location))
			return beta;
	}
	return beta;
}

int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {
	assert(moves_left != 0);
	if(0 > alpha) {
		alpha = 0;
		if(alpha >= beta)
			return alpha;
	}
	unsigned int killermove = helpers->m_killermove_player[moves_left - 1];
	if(TurnPlayer(field, moves_left, alpha, beta, helpers, killermove)) {
		++helpers->m_killermove_player_hit;
		return alpha;
	}
	++helpers->m_killermove_player_miss;
	for(unsigned int direction = 0; direction < 4; ++direction) {
		if(direction == killermove)
			continue;
		if(TurnPlayer(field, moves_left, alpha, beta, helpers, direction))
			return alpha;
	}
	return alpha;
}

int MinimaxBestScore(unsigned int min_moves, unsigned int max_moves, unsigned int step) {

	int score = 0;
	for(unsigned int moves = min_moves; moves <= max_moves; moves += step) {

		// initialize
		MinimaxHelpers helpers(moves);
		Field field;
		ClearField(&field);

		// run minimax
		score = CachedMinimaxTurnComputer(field, moves, SCORE_MIN, SCORE_MAX, &helpers);

		// print stats
		std::cout << "Moves: " << moves << ", Best score: " << score << std::endl;
		/*std::cout << "**** Killermove computer - hit=" << helpers.m_killermove_computer_hit
				  << " miss=" << helpers.m_killermove_computer_miss
				  << std::endl;*/
		std::cout << "**** Killermove player - hit=" << helpers.m_killermove_player_hit
				  << " miss=" << helpers.m_killermove_player_miss
				  << std::endl;
		std::cout << "**** Cache computer - hit=" << helpers.m_cache_computer_hit
				  << " partial=" << helpers.m_cache_computer_partial
				  << " miss=" << helpers.m_cache_computer_miss
				  << " deadend=" << helpers.m_dead_ends
				  << std::endl;

	}

	return score;
}

unsigned int MinimaxBestMove(const Field& field, unsigned int max_moves, bool computer_starts) {
	MinimaxHelpers helpers(max_moves);

	int alpha = SCORE_MIN, beta = SCORE_MAX;
	unsigned int best_move= -1;

	if(computer_starts) {
		for(unsigned int location = 0; location < FIELD_SIZE * FIELD_SIZE; ++location) {
			Field newfield;
			if(InsertBlock(&newfield, field, location, 1)) {
				int score = MinimaxTurnPlayer(newfield, max_moves, alpha, beta, &helpers);
				if(score < beta) {
					beta = score;
					best_move = location;
				}
			}
		}
	} else {
		for(unsigned int direction = 0; direction < 4; ++direction) {
			Field newfield;
			int score = 0;
			if(ApplyGravity(&newfield, field, (enum_direction) direction, &score)) {
				score += CachedMinimaxTurnComputer(newfield, max_moves - 1, alpha - score, beta - score, &helpers);
				if(score > alpha) {
					alpha = score;
					best_move = direction;
				}
			}
		}
	}

	return best_move;
}
