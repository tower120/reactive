#pragma once

#include <chrono>
#include <vector>
#include <memory>

#include <reactive/ObservableProperty.h>

class BenchmarkOwnedProperty {
public:

	struct Data_plain {
		static const int size = 1;

		std::atomic<int> x1;
		char c1[size];
		std::atomic<int> x2;
		char c2[size];
		std::atomic<int> x3;
		char c3[size];
		std::atomic<int> x4;
		char c4[size];

		Data_plain(const Data_plain& other)
			: x1{ other.x1.load() }
			, x2(other.x2.load())
			, x3(other.x3.load())
			, x4(other.x4.load())
		{}

		Data_plain(int i1, int i2, int i3, int i4)
			:/* Base()
			 , */
			x1(i1)
			, x2(i2)
			, x3(i3)
			, x4(i4)
		{}

		int sum() {
			return x1 + x2 + x3 + x4;
		}
	};

	struct Data {
		reactive::details::ObservableProperty<int, reactive::nonblocking_atomic> x1, x2, x3, x4;
		
		Data(int i1, int i2, int i3, int i4)
			: x1(i1)
			, x2(i2)
			, x3(i3)
			, x4(i4)
		{}

		int sum() {
			return x1.getCopy() + x2.getCopy() + x3.getCopy() + x4.getCopy();
		}
	};


	struct Data2 {
		reactive::ObservableProperty<int, reactive::nonblocking_atomic> x1, x2, x3, x4;

		Data2(int i1, int i2, int i3, int i4)
			: x1(i1)
			, x2(i2)
			, x3(i3)
			, x4(i4)
		{}

		int sum() {
			return x1.getCopy() + x2.getCopy() + x3.getCopy() + x4.getCopy();
		}
	};


	const int count = 100'000;

	void benchmark_plain() {
		using namespace std::chrono;

		std::vector< Data_plain > list;
		list.reserve(count);
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			for (int i = 0; i < count; ++i) {
				list.emplace_back(i, i + 1, i + 2, i + 3);
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Added in : " << duration << std::endl;
		}

		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			int sum = 0;
			for (int i = 0; i < count; ++i) {
				sum += list[i].sum();
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Traverse in : " << duration << std::endl;
			std::cout << sum << std::endl;
		}
	}

	void benchmark_owned() {
		using namespace std::chrono;

		std::vector< std::shared_ptr<Data> > list;
		list.reserve(count);
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			for (int i = 0; i < count; ++i) {
				list.emplace_back(std::make_shared<Data>(i, i + 1, i + 2, i + 3));
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Added in : " << duration << std::endl;
		}

		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			int sum = 0;
			for (int i = 0; i < count; ++i) {
				sum += list[i]->sum();
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Traverse in : " << duration << std::endl;
			std::cout << sum << std::endl;
		}
	}

	void benchmark_properties() {
		using namespace std::chrono;

		std::vector< Data2 > list;
		list.reserve(count);
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			for (int i = 0; i < count; ++i) {
				list.emplace_back(i, i + 1, i + 2, i + 3);
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Added in : " << duration << std::endl;
		}
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			int sum = 0;
			for (int i = 0; i < count; ++i) {
				sum += list[i].sum();
			}

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<microseconds>(t2 - t1).count();
			std::cout << "Traverse in : " << duration << std::endl;
			std::cout << sum << std::endl;
		}
	}


	void benchmark_all() {
		std::cout << "Plain" << std::endl;
		benchmark_plain();
		std::cout << std::endl;

		std::cout << "Owned" << std::endl;
		benchmark_owned();
		std::cout << std::endl;

		std::cout << "Properties" << std::endl;
		benchmark_properties();
		std::cout << std::endl;
	}
};