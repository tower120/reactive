#ifndef TEST_TESTREACTIVEPROPERTY_H
#define TEST_TESTREACTIVEPROPERTY_H

#include <reactive/ReactiveProperty.h>


class TestReactiveProperty{
public:
    void test_simple(){
        reactive::ObservableProperty<int> x = 1;
        reactive::ObservableProperty<int> y = 2;
        reactive::ReactiveProperty<int> summ {-1};


        reactive::ReactiveProperty<int> summ_copy = 24;

        /*summ_copy = summ;
        std::cout << summ.getCopy() << std::endl;*/

        summ.set([](int x, int y){
            return x+y;
        }, x, y);
        std::cout << summ.getCopy() << std::endl;

        summ.set([](int x, int y){
            return x-y;
        }, x, y);
        x = 4;
        std::cout << summ.getCopy() << std::endl;

        summ = 10;
        x = -1;
        std::cout << summ.getCopy() << std::endl;


        {
            auto summ_ptr = summ.write_lock();
            *summ_ptr = 5;
        }
        std::cout << summ.getCopy() << std::endl;
        std::cout << summ_copy.getCopy() << std::endl;
    }

    void test_update(){
        reactive::ObservableProperty<int> x = 1;
        reactive::ObservableProperty<int> y = 2;
        reactive::ReactiveProperty<std::pair<int, int>> vec2 = {10, 20};

        vec2 += [](const std::pair<int, int>& pair){
            std::cout << pair.first << ", " << pair.second << std::endl;
        };

        vec2.update([](std::pair<int, int>& pair, int x){
            pair.first = x;
        }, x);

        x = 100;

        //x.pulse();
    }

    void test_all(){
        //test_simple();
        test_update();
    }
};

#endif //TEST_TESTREACTIVEPROPERTY_H
