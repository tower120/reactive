#ifndef TEST_TESTOBSERVABLEPROPERTY_H
#define TEST_TESTOBSERVABLEPROPERTY_H

#include <iostream>
#include <thread>

#include <reactive/ObservableProperty.h>

class TestObservableProperty{
public:
    void test_simple(){
        reactive::ObservableProperty<int> len{1};

        len += [](int len){
            std::cout << "len = " << len << std::endl;
        };

        len = 10;
        len = 20;

        std::cout << len.getCopy() << std::endl;
    }

	void test_unsubscribe() {
		reactive::ObservableProperty<float> len{ 1 };

		reactive::Delegate<int> delegate2 = [](int len) {
			std::cout << std::showpoint << "len = " << len << std::endl;
		};
		len += delegate2;

		len = 10;
		len = 20;
		/*
		std::cout << len.getCopy() << std::endl;*/
	}

    void test_in_thread(){
        {
            reactive::ObservableProperty<int> len{1};

            std::thread t{[len_ptr = len.weak_ptr()]() {
                while (true) {
                    reactive::ObservableProperty<int> len{len_ptr};
                    if (!len) {
                        std::cout << "len died." << std::endl;
                        return;
                    }

                    std::cout << "len =" << len.getCopy() << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }};
            t.detach();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    void test_read_lock(){
        struct Pos{
            int x,y;
            Pos(const Pos&) = delete;
            Pos(Pos&&) = default;
            Pos(int x, int y) : x(x), y(y){}
        };

        reactive::ObservableProperty<Pos> len{Pos(1,2)};

        {
            auto len_ptr = len.lock();
            std::cout
                << len_ptr->x
                << len_ptr->y;
        }
    }

    void test_write_lock(){
        struct Pos{
            int x,y;
			Pos() {};
			//Pos() noexcept {};
			//Pos(const Pos&) = delete;
            Pos(int x, int y) : x(x), y(y){}
        };

        reactive::ObservableProperty<Pos> len{1,2};

        len += [](const Pos& len){
            std::cout << "len = " << len.x << "," << len.y << std::endl;
        };

        {
            auto len_ptr = len.write_lock();

            len_ptr->x = 100;
            len_ptr->y = 200;
        }
    }


    void test_copy(){
        struct Pos{
            int x,y;
			Pos() noexcept {}
			//Pos(const Pos&) = delete;
            Pos(int x, int y) : x(x), y(y){}
        };

        reactive::ObservableProperty<Pos> pos1{1,2};

        pos1 += [](const Pos& len){
            std::cout << "pos1 = " << len.x << "," << len.y << std::endl;
        };

        auto pos2 = pos1;
        {
            auto pos2_ptr = pos2.lock();
            std::cout << "pos2 = "
                    << pos2_ptr->x  << ","
                    << pos2_ptr->y
                    << std::endl;
        }

        pos1 = Pos{3,4};
    }

    void test_size(){
        struct T { char v[32]; };
        //using T = long;
        reactive::details::ObservableProperty<T, reactive::nonblocking_atomic> ia;
        reactive::details::ObservableProperty<T, reactive::nonblocking> in;
        reactive::details::ObservableProperty<T, reactive::blocking> ib;

        std::cout << sizeof(T) << std::endl;
        std::cout << sizeof(ia) << std::endl;
        std::cout << sizeof(in) << std::endl;
        std::cout << sizeof(ib) << std::endl;
        std::cout << std::endl;
        std::cout << sizeof(reactive::Event<int>) << std::endl;


        struct Test
        {
            std::atomic<T> value;
            reactive::Event<int> e;
        };

        std::cout << sizeof(Test) << std::endl;
    }

    void test_all(){
        //test_simple();
		test_unsubscribe();
		//test_in_thread();
        //test_read_lock();
        //test_write_lock();
        //test_copy();

        //test_size();
    }
};

#endif //TEST_TESTOBSERVABLEPROPERTY_H
