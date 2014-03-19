#include "Minimax.h"

#include <cassert>

#include <iostream>
#include <limits>
#include <vector>
#include <unordered_map>

#define SCORE_MIN -1000000000
#define SCORE_MAX 1000000000

struct PositionComputer {
	Field field;
	unsigned int moves_left;
	bool operator==(const PositionComputer& other) const {
		return (field == other.field && moves_left == other.moves_left);
	}
};
struct PositionComputerResult {
	int normalized_score_lo, normalized_score_hi;
};

namespace std {
template<>
struct hash<PositionComputer> {
	size_t operator()(const PositionComputer& pos) const {
		// FNV-1a hash
		uint64_t h = 14695981039346656037ull;
		for(unsigned int i = 0; i < FIELD_SIZE; ++i) {
			for(unsigned int j = 0; j < FIELD_SIZE; ++j) {
				h = (h ^ pos.field.cells[i][j]) * 1099511628211ull;
			}
		}
		h = (h ^ pos.moves_left) * 1099511628211ull;
		return h;
	}
};
}

struct MinimaxHelpers {
	std::vector<unsigned int> m_killermove_computer;
	std::vector<unsigned int> m_killermove_player;
	std::unordered_map<PositionComputer, PositionComputerResult> m_positioncache_computer;
	unsigned int m_positioncache_computer_hit, m_positioncache_computer_partial, m_positioncache_computer_miss;
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

	// check the cache
	auto cache = helpers->m_positioncache_computer.insert(std::make_pair<PositionComputer, PositionComputerResult>({field, moves_left}, {0, 0}));
	PositionComputerResult &cache_result = cache.first->second;
	if(cache.second) {
		++helpers->m_positioncache_computer_miss;

		int result = MinimaxTurnComputer(field, moves_left, alpha, beta, helpers);
		if(result <= alpha) {
			cache_result.normalized_score_lo = SCORE_MIN;
			cache_result.normalized_score_hi = result;
		} else if(result >= beta) {
			cache_result.normalized_score_lo = result;
			cache_result.normalized_score_hi = SCORE_MAX;
		} else {
			cache_result.normalized_score_lo = result;
			cache_result.normalized_score_hi = result;
		}
		return result;

	} else {
		++helpers->m_positioncache_computer_hit;

		if(cache_result.normalized_score_hi <= alpha)
			return cache_result.normalized_score_hi;
		if(cache_result.normalized_score_lo >= beta)
			return cache_result.normalized_score_lo;
		if(cache_result.normalized_score_lo > alpha && cache_result.normalized_score_hi < beta)
			return cache_result.normalized_score_lo;

		--helpers->m_positioncache_computer_hit;
		++helpers->m_positioncache_computer_partial;

		if(cache_result.normalized_score_lo > alpha)
			alpha = cache_result.normalized_score_lo;
		if(cache_result.normalized_score_hi < beta)
			beta = cache_result.normalized_score_hi;

		int result = MinimaxTurnComputer(field, moves_left, alpha, beta, helpers);
		assert(result >= cache_result.normalized_score_lo);
		assert(result <= cache_result.normalized_score_hi);
		if(result <= alpha) {
			if(cache_result.normalized_score_hi > result)
				cache_result.normalized_score_hi = result;
		} else if(result >= beta) {
			if(cache_result.normalized_score_lo < result)
				cache_result.normalized_score_lo = result;
		} else {
			if(cache_result.normalized_score_lo < result)
				cache_result.normalized_score_lo = result;
			if(cache_result.normalized_score_hi > result)
				cache_result.normalized_score_hi = result;
		}
		return result;

	}

}

int MinimaxTurnComputer(const Field& field, unsigned int moves_left, int alpha, int beta, MinimaxHelpers* helpers) {
	/*if(moves_left == 0)
		return 1000000;*/
	assert(moves_left != 0);
	unsigned int killermove = helpers->m_killermove_computer[moves_left - 1];
	if(TurnComputer(field, moves_left, alpha, beta, helpers, killermove))
		return beta;
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
		//assert(alpha < beta); // this branch is never pruned
		if(alpha >= beta) {
			std::cerr << "no-move pruned!" << std::endl;
			return alpha;
		}
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

int MinimaxBestScore(unsigned int moves) {

	// initialize killer moves
	MinimaxHelpers helpers;
	helpers.m_killermove_computer.resize(moves, 0);
	helpers.m_killermove_player.resize(moves, 0);
	helpers.m_positioncache_computer_hit = 0;
	helpers.m_positioncache_computer_partial = 0;
	helpers.m_positioncache_computer_miss = 0;

	// initialize field
	Field field;
	ClearField(&field);

	// run minimax
	int score = CachedMinimaxTurnComputer(field, moves, SCORE_MIN, SCORE_MAX, &helpers);

	// print stats
	std::cerr << "**** Cache size: " << helpers.m_positioncache_computer.size()
			  << ", Hit: " << helpers.m_positioncache_computer_hit
			  << ", Partial: " << helpers.m_positioncache_computer_partial
			  << ", Miss: " << helpers.m_positioncache_computer_miss
			  << std::endl;

	return score;
}
