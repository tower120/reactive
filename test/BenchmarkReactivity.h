#pragma once

#include <vector>
#include <chrono>

#include <reactive/blocking.h>
#include <reactive/ReactiveProperty.h>
#include <reactive/non_thread_safe/ReactiveProperty.h>
#include <reactive/non_thread_safe/ObservableProperty.h>

class BenchmarkReactivity {
public:
	const int count = 100'000;

	template<class T, class R>
	struct Data {
		T x1, x2, x3, x4;
		R sum;

		Data() {
			sum.set([](auto&& x1, auto&& x2, auto&& x3, auto&& x4) {
				return x1 + x2 + x3 + x4;
			}, x1, x2, x3, x4);
		}

		template<class I1, class I2, class I3, class I4>
		void update(I1&& x1, I2&& x2, I3&& x3, I4&& x4) {
			this->x1 = x1;
			this->x2 = x2;
			this->x3 = x3;
			this->x4 = x4;
		}
	};

	template<class Container>
	void benchmark_fill(Container& container) {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		for (int i = 0; i < count; i++) {
			container.emplace_back();
		}		

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(t2 - t1).count();
		std::cout << "filled in : " << duration << std::endl;
	}

	template<class Container>
	void benchmark_update(Container& container) {
		using namespace std::chrono;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		long long sum = 0;
		for (int i = 0; i < count; i++) {
			container[i].update(i, i+1, i+2, i+3);
			sum += container[i].sum.getCopy();
		}

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(t2 - t1).count();
		std::cout << "updated in : " << duration
				  << " (" << sum << ")"
				  << std::endl;
	}

	void benchmark_all() {
		// There will be 400'000 update calls in total.
		using T = long long;
		{
			std::cout << "Test threaded."  << std::endl;
			using Element = Data<reactive::ObservableProperty<T>, reactive::ReactiveProperty<T> >;
			std::vector<Element> list;
			benchmark_fill(list);
			benchmark_update(list);
			std::cout << "---"  << std::endl;
		}
		{
			std::cout << "Test non-threaded."  << std::endl;
			using Element = Data<reactive::non_thread_safe::ObservableProperty<T>, reactive::non_thread_safe::ReactiveProperty<T> >;
			std::vector<Element> list;
			benchmark_fill(list);
			benchmark_update(list);
			std::cout << "---"  << std::endl;
		}
		{
			std::cout << "Test mix."  << std::endl;
			using Element = Data<reactive::non_thread_safe::ObservableProperty<T>, reactive::ReactiveProperty<T> >;
			std::vector<Element> list;
			benchmark_fill(list);
			benchmark_update(list);
			std::cout << "---"  << std::endl;
		}

		char ch;
		std::cin >> ch;
	}
};