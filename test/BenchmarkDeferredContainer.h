#pragma once

#include <chrono>

#include <reactive/details/utils/DeferredForwardContainer.h>

class BenchmarkDeferredContainer {
public:
	const int count = 100'000;

	// Somehow... There is no difference at all
	using T = long long int;
	using SingleThreadedList = utils::DeferredForwardContainer<T, T, threading::dummy_mutex, threading::dummy_mutex>;
	using MultiThreadedList  = utils::DeferredForwardContainer<T>;
	SingleThreadedList list;
	std::vector<T> vec;

	void benchmark_fill() {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		for (int i = 0; i < count; i++) {
			list.emplace(i);
		}

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(t2 - t1).count();
		std::cout << "filled in : " << duration << std::endl;
	}

	void benchmark_foreach() {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();


		long long int sum = 0;
		for (int i = 0; i < count; i++) {
			list.foreach([&](int i) {
				sum += i;
			});
		}

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(t2 - t1).count();
		std::cout << sum << std::endl;
		std::cout << "foreached in : " << duration << std::endl;
	}



	void benchmark_vec_fill() {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		for (int i = 0; i < count; i++) {
			vec.emplace_back(i);
		}

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(t2 - t1).count();
		std::cout << "vec filled in : " << duration << std::endl;
	}

	void benchmark_vec_foreach() {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();


		long long int sum = 0;
		for (int i = 0; i < count; i++) {
			for(auto&& i : vec){
				sum += i;
			}
		}

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(t2 - t1).count();
		std::cout << sum << std::endl;
		std::cout << "vec foreached in : " << duration << std::endl;
	}

	void benchmark_all() {
		benchmark_fill();
		benchmark_foreach();
		benchmark_vec_fill();
		benchmark_vec_foreach();
	}
};