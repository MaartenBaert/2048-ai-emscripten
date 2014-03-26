#include "Minimax.h"

// it's hard to pass JS arrays to C++, this is simpler ;)
extern "C"
unsigned int JS_MinimaxBestMove(cell_t f00, cell_t f01, cell_t f02, cell_t f03,
								cell_t f10, cell_t f11, cell_t f12, cell_t f13,
								cell_t f20, cell_t f21, cell_t f22, cell_t f23,
								cell_t f30, cell_t f31, cell_t f32, cell_t f33) {
	Field field = {{
		{f00, f01, f02, f03},
		{f10, f11, f12, f13},
		{f20, f21, f22, f23},
		{f30, f31, f32, f33},
	}};
	HeuristicParameters parameters;
	GetDefaultHeuristicParameters(&parameters);
	return MinimaxBestMove(field, 6, parameters);
}
