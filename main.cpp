#include "2048.h"
#include "Minimax.h"

#include <cstdlib>
#include <ctime>

#include <future>
#include <iostream>
#include <random>

int MinimaxPlayTest() {
	std::cout << "---- BEGIN MinimaxPlayTest ----" << std::endl;

	std::mt19937 rng(clock());

	Field a, b;
	ClearField(&a);

	int score = 0;
	for(unsigned int move = 0; move < 1000000; ++move) {
		//std::cout << "Move: " << move << ", Score: " << score << std::endl;

		bool done = false;
		for(unsigned int i = 0; i < 1000; ++i) {
			if(InsertBlock(&b, a, rng() % (FIELD_SIZE * FIELD_SIZE), (rng() % 10 == 0)? 2 : 1)) {
				done = true;
				break;
			}
		}
		if(!done) {
			std::cout << "Can't insert!" << std::endl;
			break;
		}
		//PrintField(b);

		//unsigned int usedcells = FIELD_SIZE * FIELD_SIZE - CountFreeCells(b);
		//unsigned int moves = 8 + ((usedcells > 12)? usedcells - 12 : 0);
		//unsigned int moves = 8 + score / 10000;
		unsigned int direction = MinimaxBestMove(b, 8, false);
		if(direction == (unsigned int) -1) {
			std::cout << "No possible move!" << std::endl;
			break;
		}
		if(!ApplyGravity(&a, b, (enum_direction) direction, &score)) {
			std::cout << "Move: " << move << ", Score: " << score << std::endl;
			std::cout << "Can't move!" << std::endl;
			break;
		}
		//PrintField(a);

	}

	std::cout << "---- END MinimaxPlayTest ----" << std::endl;
	return score;
}

void MinimaxPerfTest() {
	MinimaxBestScore(1, 20, 2);
}

#define NUM_PLAYS 200

int main() {

	//MinimaxPlayTest();
	//MinimaxTest();

	std::future<int> scores[NUM_PLAYS];
	for(unsigned int p = 0; p < NUM_PLAYS; ++p) {
		scores[p] = std::async(MinimaxPlayTest);
	}
	for(unsigned int p = 0; p < NUM_PLAYS; ++p) {
		scores[p].wait();
	}

	std::cout << "scores = array([";
	for(unsigned int p = 0; p < NUM_PLAYS; ++p) {
		std::cout << scores[p].get() << ", ";
	}
	std::cout << "])" << std::endl;

	std::cout << "Done!" << std::endl;
	return 0;
}

