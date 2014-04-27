#include "2048.h"
#include "Minimax.h"

#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

unsigned int g_threadcount = 0;
unsigned int g_threadcount_max = std::thread::hardware_concurrency();
std::mutex g_threadcount_mutex;
std::condition_variable g_threadcount_cv;

template<typename Func, typename... Args>
typename std::result_of<Func(Args...)>::type JobWrapper(Func func, Args... args) {

	// run the job
	typename std::result_of<Func(Args...)>::type result = func(std::forward<Args>(args)...);

	// decrement the thread count
	std::unique_lock<std::mutex> lock(g_threadcount_mutex);
	--g_threadcount;
	g_threadcount_cv.notify_one();

	return result;
}

template<typename Func, typename... Args>
std::future<typename std::result_of<Func(Args...)>::type> StartJob(Func func, Args... args) {

	// wait until the thread count drops below the maximum
	std::unique_lock<std::mutex> lock(g_threadcount_mutex);
	while(g_threadcount >= g_threadcount_max) {
		g_threadcount_cv.wait(lock);
	}

	// increment the thread count
	++g_threadcount;

	// start the thread
	return std::async(std::launch::async, JobWrapper<Func, Args...>,
					  func, std::forward<Args>(args)...);

}

#define SEARCH_DEPTH 6

#define RUN_TUNE 0
#define TUNE_PLAYS 2000
#define TUNE_POPULATION 100
#define TUNE_TOURNAMENT 10
#define TUNE_LATENCY 30

#define BATCH_PLAYS 400

unsigned int MinimaxPlayTest(bool use_penalty, const HeuristicParameters& parameters) {
	((void) use_penalty);

	std::mt19937 rng(clock());

	Field a, b;
	ClearField(&a);

	auto t1 = std::chrono::high_resolution_clock::now();

	unsigned int moves = 0, score = 0;
	for( ; ; ) {
		//if(print)
		//	std::cout << "Move: " << moves << ", Score: " << score << std::endl;

		bool done = false;
		for(unsigned int i = 0; i < 1000; ++i) {
			if(InsertBlock(&b, a, rng() % (FIELD_SIZE * FIELD_SIZE), (rng() % 10 == 0)? 2 : 1)) {
				done = true;
				break;
			}
		}
		if(!done) {
			std::cout << "Can't insert!" << std::endl;
			exit(1);
			break;
		}
		//if(print)
		//	PrintField(b);

		unsigned int direction = MinimaxBestMove(b, SEARCH_DEPTH, parameters);
		if(direction == (unsigned int) -1)
			break;
		if(!ApplyGravity(&a, b, (enum_direction) direction, &score)) {
			std::cout << "Invalid move!" << std::endl;
			exit(1);
			break;
		}
		//if(print)
		//	PrintField(a);

		++moves;
	}

	auto t2 = std::chrono::high_resolution_clock::now();
	unsigned int time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	unsigned int time_per_move = time / moves;
	//unsigned int max_time_per_move = 2000;
	//unsigned int penalty = (time_per_move > max_time_per_move)? (time_per_move - max_time_per_move) * 1 : 0;

	std::cout << "Game over - Move: " << moves << ", Time per move: " << time_per_move /*<< ", Penalty: " << penalty*/ << ", Score: " << score << std::endl;

	return score;
	//return (use_penalty)? ((score > penalty)? score - penalty : 0) : score;
}

struct TuneElement {
	HeuristicParameters m_parameters;
	unsigned int m_score;
};

int Mutate(int value, int low, int high, int step, std::mt19937& rng) {
	if(step == -1)
		return value;
	value += rng() % step;
	value -= rng() % step;
	if(rng() % 10 == 0)
		++value;
	if(rng() % 10 == 0)
		--value;
	if(value < low)
		return low;
	if(value > high)
		return high;
	return value;
}

void MinimaxTuneTest() {

	std::mt19937 rng(clock());

	// create initial population
	TuneElement population[TUNE_POPULATION];
	for(unsigned int i = 0; i < TUNE_POPULATION; ++i) {
		GetDefaultHeuristicParameters(&population[i].m_parameters);
		population[i].m_score = 15000; // guess, should be relatively low
	}

	// simulate plays
	TuneElement plays[TUNE_PLAYS];
	std::future<unsigned int> futures[TUNE_LATENCY];
	for(unsigned int p = 0; p < TUNE_PLAYS + TUNE_LATENCY; ++p) {
		std::cout << "Tune progress: " << 100 * p / (TUNE_PLAYS + TUNE_LATENCY) << "%" << std::endl;

		// add completed play to the population
		if(p >= TUNE_LATENCY) {
			plays[p - TUNE_LATENCY].m_score = futures[p % TUNE_LATENCY].get();
			population[(p - TUNE_LATENCY) % TUNE_POPULATION] = plays[p - TUNE_LATENCY];
		}

		// tournament selection
		TuneElement best1, best2;
		GetDefaultHeuristicParameters(&best1.m_parameters);
		GetDefaultHeuristicParameters(&best2.m_parameters);
		best1.m_score = 0;
		best2.m_score = 0;
		for(unsigned int t = 0; t < TUNE_TOURNAMENT; ++t) {
			unsigned int sel1 = rng() % TUNE_POPULATION;
			if(population[sel1].m_score > best1.m_score)
				best1 = population[sel1];
			unsigned int sel2 = rng() % TUNE_POPULATION;
			if(population[sel2].m_score > best2.m_score)
				best2 = population[sel2];
		}

		// create winner
		HeuristicParameters winner;
		std::cout << "Winner (" << best1.m_score << "|" << best2.m_score << "): ";
		for(unsigned int i = 0; i < PARAM_COUNT; ++i) {
			winner.m_values[i] = (best1.m_parameters.m_values[i] + best2.m_parameters.m_values[i] + (rng() & 1)) / 2;
			std::cout << winner.m_values[i] << " ";
		}
		std::cout << std::endl;

		if(p < TUNE_PLAYS) {

			// do some mutations
			for(unsigned int i = 0; i < PARAM_COUNT; ++i) {
				winner.m_values[i] = Mutate(winner.m_values[i], PARAMETERS_MIN[i], PARAMETERS_MAX[i], PARAMETERS_STEP[i], rng);
			}

			// start the job
			plays[p].m_parameters = winner;
			futures[p % TUNE_LATENCY] = StartJob(MinimaxPlayTest, true, winner);

		}

	}

	std::cout << "scores = array([\n\t";
	for(unsigned int p = 0; p < TUNE_PLAYS; ++p) {
		std::cout << plays[p].m_score;
		if(p != TUNE_PLAYS - 1) {
			if(p % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

	// calculate population average
	HeuristicParameters population_average;
	std::cout << "Population average: ";
	for(unsigned int i = 0; i < PARAM_COUNT; ++i) {
		population_average.m_values[i] = 0;
		for(unsigned int p = 0; p < TUNE_POPULATION; ++p) {
			population_average.m_values[i] += population[p].m_parameters.m_values[i];
		}
		population_average.m_values[i] = (population_average.m_values[i] + TUNE_POPULATION / 2) / TUNE_POPULATION;
		std::cout << population_average.m_values[i] << " ";
	}
	std::cout << std::endl;

}

void MinimaxBatchTest() {

	HeuristicParameters parameters;
	GetDefaultHeuristicParameters(&parameters);

	std::future<unsigned int> scores[BATCH_PLAYS];
	for(unsigned int p = 0; p < BATCH_PLAYS; ++p) {
		std::cout << "Batch progress: " << 100 * p / BATCH_PLAYS << "%" << std::endl;
		scores[p] = StartJob(MinimaxPlayTest, false, parameters);
	}

	std::cout << "Finishing ..." << std::endl;
	for(unsigned int p = 0; p < BATCH_PLAYS; ++p) {
		scores[p].wait();
	}

	std::cout << "scores = array([\n\t";
	for(unsigned int p = 0; p < BATCH_PLAYS; ++p) {
		std::cout << scores[p].get();
		if(p != BATCH_PLAYS - 1) {
			if(p % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

}

int main() {

	//MinimaxPerfTest();
	//MinimaxPlayTest(true);

#if RUN_TUNE
	MinimaxTuneTest();
#else
	MinimaxBatchTest();
#endif

	std::cout << "Done!" << std::endl;
	return 0;
}

