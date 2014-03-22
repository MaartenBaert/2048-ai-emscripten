#include "2048.h"
#include "Minimax.h"

#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

unsigned int g_threadcount = 0;
unsigned int g_threadcount_max = std::thread::hardware_concurrency() + 1;
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

#define BATCH_PLAYS 100

#define TUNE_PLAYS 300
#define TUNE_POPULATION 50
#define TUNE_LATENCY 15

void MinimaxPerfTest() {
	HeuristicParameters parameters;
	GetDefaultHeuristicParameters(&parameters);
	MinimaxBestScore(1, 20, 2, parameters);
}

unsigned int MinimaxPlayTest(bool print, const HeuristicParameters& parameters) {

	std::mt19937 rng(clock());

	Field a, b;
	ClearField(&a);

	unsigned int score = 0, extended = 0;
	for(unsigned int move = 0; move < 1000000; ++move) {
		if(print)
			std::cout << "Move: " << move << ", Score: " << score << std::endl;

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
		if(print)
			PrintField(b);

		unsigned int moves = SEARCH_DEPTH;
		if(CountFreeCells(b) < 3) {
			++moves;
			++extended;
		}

		unsigned int direction = MinimaxBestMove(b, moves, parameters);
		if(direction == (unsigned int) -1) {
			std::cout << "Game over - Move: " << move << ", Extended: " << extended << ", Score: " << score << std::endl;

			break;
		}
		if(!ApplyGravity(&a, b, (enum_direction) direction, &score)) {
			std::cout << "Invalid move!" << std::endl;
			exit(1);
			break;
		}
		if(print)
			PrintField(a);

	}

	return score;
}

struct TuneElement {
	HeuristicParameters m_parameters;
	unsigned int m_score;
	bool operator<(const TuneElement& other) const {
		return (m_score > other.m_score);
	}
};

int Mutate(int value, int low, int high, std::mt19937& rng) {
	value += ((int) (rng()) % (2 * value + 1) - value) / 20;
	if(rng() % 5 == 0)
		++value;
	if(rng() % 5 == 0)
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
		population[i].m_score = 10000; // guess, should be relatively low
	}

	// simulate plays
	HeuristicParameters play_parameters[TUNE_PLAYS];
	std::future<unsigned int> play_scores[TUNE_PLAYS];
	for(unsigned int p = 0; p < TUNE_PLAYS + TUNE_LATENCY; ++p) {
		std::cout << "Tune progress: " << 100 * p / (TUNE_PLAYS + TUNE_LATENCY) << "%" << std::endl;

		// add completed play to the population
		if(p >= TUNE_LATENCY) {

			// remove element with the lowest score from the population
			// (heap removes the highest element, highest element means lowest score)
			std::pop_heap(population, population + TUNE_POPULATION);

			// replace it with the new one
			population[TUNE_POPULATION - 1].m_parameters = play_parameters[p - TUNE_LATENCY];
			population[TUNE_POPULATION - 1].m_score = play_scores[p - TUNE_LATENCY].get();
			std::push_heap(population, population + TUNE_POPULATION);

		}

		// get the average of the current population
		uint64_t sum_score_stillalive = 0;
		uint64_t sum_score_freecell = 0;
		uint64_t sum_score_centerofmass = 0;
		uint64_t sum_weight = 0;
		for(unsigned int i = 0; i < TUNE_POPULATION; ++i) {
			sum_score_stillalive += population[i].m_score * population[i].m_parameters.m_score_stillalive;
			sum_score_freecell += population[i].m_score * population[i].m_parameters.m_score_freecell;
			sum_score_centerofmass += population[i].m_score * population[i].m_parameters.m_score_centerofmass;
			sum_weight += population[i].m_score;
		}
		HeuristicParameters average_parameters;
		average_parameters.m_score_stillalive = (sum_score_stillalive + sum_weight / 2) / sum_weight;
		average_parameters.m_score_freecell = (sum_score_freecell + sum_weight / 2) / sum_weight;
		average_parameters.m_score_centerofmass = (sum_score_centerofmass + sum_weight / 2) / sum_weight;

		if(p < TUNE_PLAYS) {

			// do some mutations
			average_parameters.m_score_stillalive = Mutate(average_parameters.m_score_stillalive, 0, 1000000, rng);
			average_parameters.m_score_freecell = Mutate(average_parameters.m_score_freecell, 0, 1000000, rng);
			average_parameters.m_score_centerofmass = Mutate(average_parameters.m_score_centerofmass, 0, 10000, rng);
			std::cout << "Mutation: "
					  << average_parameters.m_score_stillalive << " "
					  << average_parameters.m_score_freecell << " "
					  << average_parameters.m_score_centerofmass << std::endl;

			// start the job
			play_parameters[p] = average_parameters;
			play_scores[p] = StartJob(MinimaxPlayTest, false, average_parameters);

		} else {

			std::cout << "Average: "
					  << average_parameters.m_score_stillalive << " "
					  << average_parameters.m_score_freecell << " "
					  << average_parameters.m_score_centerofmass << std::endl;

		}

	}

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

	//MinimaxTuneTest();
	MinimaxBatchTest();

	std::cout << "Done!" << std::endl;
	return 0;
}

