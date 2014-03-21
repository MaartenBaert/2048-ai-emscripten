#include "2048.h"
#include "Minimax.h"

#include <cstdlib>
#include <ctime>

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
typename std::result_of<Func(Args...)>::type JobWrapper(Func func, Args&&... args) {

	// run the job
	typename std::result_of<Func(Args...)>::type result = func(std::forward<Args>(args)...);

	// decrement the thread count
	std::unique_lock<std::mutex> lock(g_threadcount_mutex);
	--g_threadcount;
	g_threadcount_cv.notify_one();

	return result;
}

template<typename Func, typename... Args>
std::future<typename std::result_of<Func(Args...)>::type> StartJob(Func func, Args&&... args) {

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

void MinimaxPerfTest() {
	MinimaxBestScore(1, 20, 2);
}

int MinimaxPlayTest() {

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

		unsigned int direction = MinimaxBestMove(b, 6, false);
		if(direction == (unsigned int) -1) {
			std::cout << "Move: " << move << ", Score: " << score << std::endl;
			std::cout << "No possible move!" << std::endl;
			break;
		}
		if(!ApplyGravity(&a, b, (enum_direction) direction, &score)) {
			std::cout << "Invalid move!" << std::endl;
			break;
		}
		//PrintField(a);

	}

	return score;
}

#define NUM_PLAYS 200

int main() {

	//MinimaxPerfTest();

	std::future<int> scores[NUM_PLAYS];
	for(unsigned int p = 0; p < NUM_PLAYS; ++p) {
		scores[p] = StartJob(MinimaxPlayTest);
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

