/*
Copyright (c) 2014 Maarten Baert <maarten-baert@hotmail.com>

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#include "PlayTests.h"

#include "RandomSeed.h"
#include "BoardDB.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

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

void PrintBoard(Board board) {
	for(unsigned int j = 0; j < 4; ++j) {
		std::cout << "[";
		for(unsigned int i = 0; i < 4; ++i) {
			unsigned int value = GetCell(board, i, j);
			std::cout.width(5);
			std::cout  << std::right << ((value == 0)? 0 : (1 << value));
		}
		std::cout << " ]" << std::endl;
	}
	std::cout << std::endl;
}

unsigned int PlayTest(const HeuristicParameters& parameters, unsigned int rollbacks, BoardDB* boarddb, std::mutex* boarddb_mutex) {
	std::mt19937 rng(RandomSeed());

	struct HistoryData {
		Board board;
		unsigned int moves, score;
	};
	std::vector<HistoryData> history;
	std::vector<BoardScore> extra_db;

	Board board{0};

	auto t1 = std::chrono::high_resolution_clock::now();
	unsigned int moves = 0, score = 0;
	for( ; ; ) {

		history.push_back(HistoryData{board, moves, score});
		//PrintBoard(board);

		// computer
		unsigned int locations[16], location_count = 0;
		for(unsigned int location = 0; location < 16; ++location) {
			if(GetCell(board, location) == 0) {
				locations[location_count] = location;
				++location_count;
			}
		}
		if(location_count == 0) {
			std::cout << "Can't insert!" << std::endl;
			exit(1);
		}
		board = SetCell(board, locations[rng() % location_count], (rng() % 10 == 0)? 2 : 1);

		//PrintBoard(board);

		// player
		auto move = FindBestMove(board, parameters);
		extra_db.push_back(BoardScore{board, move.second});
		if(move.first == DIRECTION_NONE) {
			if(rollbacks == 0)
				break;
			--rollbacks;
			unsigned int back = 10 + rng() % 90;
			unsigned int p = (history.size() > back)? history.size() - back : 0;
			board = history[p].board;
			moves = history[p].moves;
			score = history[p].score;
			history.resize(p);
		} else {
			BoardScore result = Collapse(board, move.first);
			if(result.m_board.m_data == board.m_data) {
				std::cout << "Invalid move!" << std::endl;
				exit(1);
			}
			board = result.m_board;
			score += result.m_score * 2;
			++moves;
		}

	}
	auto t2 = std::chrono::high_resolution_clock::now();
	unsigned int time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	unsigned int time_per_move = time / moves;

	std::cout << "Game over - Move: " << moves << ", Time per move: " << time_per_move << ", Score: " << score << std::endl;

	if(boarddb != NULL) {
		std::lock_guard<std::mutex> lock(*boarddb_mutex);
		boarddb->m_results.insert(boarddb->m_results.end(), extra_db.begin(), extra_db.end());
		//unsigned int back = 100;
		//for(size_t i = (history.size() > back)? history.size() - back : 0; i < history.size(); ++i) {
		/*for(size_t i = 0; i < history.size(); ++i) {
			BoardScore result;
			result.m_board = history[i].board;
			result.m_score = score - history[i].score;
			boarddb->m_results.push_back(result);
		}*/
	}

	return score;
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

void PlayTest_Tune(unsigned int rollbacks, unsigned int plays, unsigned int population_size, unsigned int tournament_size, unsigned int latency) {
	std::mt19937 rng(RandomSeed());

	// create initial population
	std::vector<TuneElement> population(population_size);
	for(unsigned int i = 0; i < population_size; ++i) {
		GetDefaultHeuristicParameters(&population[i].m_parameters);
		population[i].m_score = 20000; // guess, should be relatively low
	}

	// simulate plays
	std::vector<TuneElement> history(plays);
	std::vector<std::future<unsigned int> > futures(latency);
	for(unsigned int p = 0; p < plays + latency; ++p) {
		std::cout << "Tune progress: " << 100 * p / (plays + latency) << "%" << std::endl;

		// add completed play to the population
		if(p >= latency) {
			history[p - latency].m_score = futures[p % latency].get();
			population[(p - latency) % population_size] = history[p - latency];
		}

		// tournament selection
		TuneElement best1, best2;
		GetDefaultHeuristicParameters(&best1.m_parameters);
		GetDefaultHeuristicParameters(&best2.m_parameters);
		best1.m_score = 0;
		best2.m_score = 0;
		for(unsigned int t = 0; t < tournament_size; ++t) {
			unsigned int sel1 = rng() % population_size;
			if(population[sel1].m_score > best1.m_score)
				best1 = population[sel1];
			unsigned int sel2 = rng() % population_size;
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

		if(p < plays) {

			// do some mutations
			for(unsigned int i = 0; i < PARAM_COUNT; ++i) {
				winner.m_values[i] = Mutate(winner.m_values[i], PARAMETERS_MIN[i], PARAMETERS_MAX[i], PARAMETERS_STEP[i], rng);
			}

			// start the job
			history[p].m_parameters = winner;
			futures[p % latency] = StartJob(PlayTest, winner, rollbacks, (BoardDB*) NULL, (std::mutex*) NULL);

		}

	}

	std::cout << "scores = array([\n\t";
	for(unsigned int p = 0; p < plays; ++p) {
		std::cout << history[p].m_score;
		if(p != plays - 1) {
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
		for(unsigned int p = 0; p < population_size; ++p) {
			population_average.m_values[i] += population[p].m_parameters.m_values[i];
		}
		population_average.m_values[i] = (population_average.m_values[i] + population_size / 2) / population_size;
		std::cout << population_average.m_values[i] << " ";
	}
	std::cout << std::endl;

}

void PlayTest_Batch(unsigned int rollbacks, unsigned int plays, BoardDB* boarddb) {

	if(boarddb != NULL && rollbacks != 0) {
		std::cout << "PlayTest_Batch: Don't combine BoardDB with rollbacks!" << std::endl;
		exit(1);
	}

	std::mutex boarddb_mutex;

	HeuristicParameters parameters;
	GetDefaultHeuristicParameters(&parameters);

	std::vector<std::future<unsigned int> > scores(plays);
	for(unsigned int p = 0; p < plays; ++p) {
		std::cout << "Batch progress: " << 100 * p / plays << "%" << std::endl;
		scores[p] = StartJob(PlayTest, parameters, rollbacks, boarddb, &boarddb_mutex);
	}

	std::cout << "Finishing ..." << std::endl;
	for(unsigned int p = 0; p < plays; ++p) {
		scores[p].wait();
	}

	std::cout << "scores = array([\n\t";
	for(unsigned int p = 0; p < plays; ++p) {
		std::cout << scores[p].get();
		if(p != plays - 1) {
			if(p % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

	PrintExpectiMaxStats();

}
