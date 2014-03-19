#include "2048.h"
#include "Minimax.h"

#include <cstdlib>
#include <ctime>

#include <iostream>

void PlayTest() {

	srand(clock());

	Field a;
	ClearField(&a);

	int score = 0;
	for(unsigned int move = 0; move < 1000000; ++move) {
		Field b;

		bool done = false;
		for(unsigned int i = 0; i < 1000; ++i) {
			if(InsertBlock(&b, a, rand() % (FIELD_SIZE * FIELD_SIZE), 1)) {
				done = true;
				break;
			}
		}
		if(!done) {
			std::cerr << "Can't insert!" << std::endl;
			break;
		}
		a = b;
		PrintField(a);

		done = false;
		for(unsigned int i = 0; i < 1000; ++i) {
			enum_direction m = (enum_direction) (rand() % 4);
			if(m == DIRECTION_UP && i < 500)
				m = DIRECTION_DOWN;
			if(ApplyGravity(&b, a, m, &score)) {
				done = true;
				break;
			}
		}
		if(!done) {
			std::cerr << "Can't move!" << std::endl;
			break;
		}
		a = b;
		PrintField(a);

		std::cerr << "Moves: " << move + 1 << ", Score: " << score << std::endl;
	}

}

void MinimaxTest() {
	MinimaxBestScore(25);
}

int main() {

	//PlayTest();
	MinimaxTest();

	std::cerr << "Done!" << std::endl;
	return 0;
}

