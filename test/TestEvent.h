#ifndef TEST_TESTEVENT_H
#define TEST_TESTEVENT_H

#include <iostream>

#include <reactive/Event.h>

class TestEvent{
public:
    void test_simple(){
        reactive::Event<int, int> onMove;

        onMove += [](int x, int y){
            std::cout << "position = " << x << "," << y << std::endl;
        };

        onMove(1,2);
        onMove(10,20);
    }

    void test_unsubscribe_from_observer(){
        reactive::Event<int, int> onMove;

        onMove += [](int x, int y){
            std::cout << "position = " << x << "," << y << std::endl;
        };

        reactive::Delegate<int, int> another_observer = [&](int x, int y){
            if (x == 10){
                onMove -= another_observer;
            }
            std::cout << "another observer " << x << "," << y << std::endl;
        };

        onMove += another_observer;


        onMove(1,2);
        onMove(10,20);
        onMove(30,50);
        onMove(100,50);

    }


	void test_action() {
		reactive::Event<int> onMove;

		reactive::Delegate<int> action = [](int) {
			std::cout << "act " << std::endl;
		};

		onMove += action;

		onMove(1);
		onMove(1);
		onMove -= action;

		onMove(1);
		onMove(1);
	}



    void test_all(){
        std::cout << "Simple test." << std::endl;
        test_simple();
        std::cout << std::endl;

        std::cout << "Unsubscribe right from observer." << std::endl;
        test_unsubscribe_from_observer();
        std::cout << std::endl;

		/*std::cout << "Action test." << std::endl;
		test_action();
		std::cout << std::endl;*/
    }
};

#endif //TEST_TESTEVENT_H