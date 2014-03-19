#include "Minimax.h"

#include <cassert>

#include <iostream>
#include <limits>
#include <vector>
#include <unordered_map>

#define SCORE_MIN -1000000000
#define SCORE_MAX 1000000000

struct CachedResult {
	int score_lo, score_hi;
};

namespace std {
template<>
struct hash<Field> {
	size_t operator()(const Field& field) const {
		// FNV-1a hash
		uint64_t h = 14695981039346656037ull;
		//uint32_t h = 2166136261;
		for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
				h = (h ^ field.cells[i][j]) * 1099511628211ull;
				//h = (h ^ field.cells[i][j]) * 16777619;
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
	unsigned int m_killermove_hit, m_killermove_miss;
	unsigned int m_cache_computer_hit, m_cache_computer_partial, m_cache_computer_miss;
	unsigned int m_dead_ends;
};

int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);
int MinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);
int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers);

inline __attribute__((always_inline))
bool TurnComputer(const Field& field, unsigned int moves_left, int alpha, int& beta, MinimaxHelpers* helpers, unsigned int location) {
	Field newfield;
	if(InsertBlock(&newfield, field, location, 1)) {
		int score = MinimaxTurnPlayer(newfield, moves_left, alpha, beta, helpers);
		if(score < beta) {
			helpers->m_killermove_computer[moves_left - 1] = location;
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
	int newscore = 0;
	if(ApplyGravity(&newfield, field, (enum_direction) direction, &newscore)) {
		newscore += CachedMinimaxTurnComputer(newfield, moves_left - 1, alpha - newscore, beta - newscore, helpers);
		if(newscore > alpha) {
			helpers->m_killermove_player[moves_left - 1] = direction;
			alpha = newscore;
			if(alpha >= beta)
				return true;
		}
	}
	return false;
}

int CachedMinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {

	if(moves_left == 0)
		return 1000000;

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

		if(cache_result.score_hi < 1000000)
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

		if(cache_result.score_hi < 1000000)
			++helpers->m_dead_ends;

		return result;

	}

}

int MinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {
	/*if(moves_left == 0)
		return 1000000;*/
	assert(moves_left != 0);
	unsigned int killermove = helpers->m_killermove_computer[moves_left - 1];
	if(TurnComputer(field, moves_left, alpha, beta, helpers, killermove)) {
		++helpers->m_killermove_hit;
		return beta;
	}
	++helpers->m_killermove_miss;
	for(unsigned int location = 0; location < FIELD_SIZE * FIELD_SIZE; ++location) {
		if(location == killermove)
			continue;
		if(TurnComputer(field, moves_left, alpha, beta, helpers, location))
			return beta;
	}
	return beta;
}

int MinimaxTurnPlayer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* killermoves) {
	assert(moves_left != 0);
	if(0 > alpha) {
		alpha = 0;
		if(alpha >= beta)
			return alpha;
	}
	unsigned int killermove = killermoves->m_killermove_player[moves_left - 1];
	if(TurnPlayer(field, moves_left, alpha, beta, killermoves, killermove))
		return alpha;
	for(unsigned int direction = 0; direction < 4; ++direction) {
		if(direction == killermove)
			continue;
		if(TurnPlayer(field, moves_left, alpha, beta, killermoves, direction))
			return alpha;
	}
	return alpha;
}

int MinimaxBestScore(unsigned int min_moves, unsigned int max_moves, unsigned int step) {

	int score = 0;
	for(unsigned int moves = min_moves; moves <= max_moves; moves += step) {

		// initialize helpers
		MinimaxHelpers helpers;
		helpers.m_killermove_computer.resize(moves, 0);
		helpers.m_killermove_player.resize(moves, 0);
		helpers.m_cache_computer.resize(moves);

		// reset stats
		helpers.m_killermove_hit = 0;
		helpers.m_killermove_miss = 0;
		helpers.m_cache_computer_hit = 0;
		helpers.m_cache_computer_partial = 0;
		helpers.m_cache_computer_miss = 0;
		helpers.m_dead_ends = 0;

		// initialize field
		Field field;
		ClearField(&field);

		// run minimax
		score = CachedMinimaxTurnComputer(field, moves, SCORE_MIN, SCORE_MAX, &helpers);

		// print stats
		std::cerr << "Moves: " << moves << ", Best score: " << score << std::endl;
		std::cerr << "**** Killer move - hit=" << helpers.m_killermove_hit
				  << " miss=" << helpers.m_killermove_miss
				  << std::endl;
		std::cerr << "**** Cache - hit=" << helpers.m_cache_computer_hit
				  << " partial=" << helpers.m_cache_computer_partial
				  << " miss=" << helpers.m_cache_computer_miss
				  << " deadend=" << helpers.m_dead_ends
				  << std::endl;

	}

	return score;
}
