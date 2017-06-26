#ifndef TEST_TESTOBSERVABLEPROPERTYFROMTHIS_H
#define TEST_TESTOBSERVABLEPROPERTYFROMTHIS_H

#include <chrono>
#include <future>

#include <reactive/ObservablePropertyFromThis.h>
#include <reactive/ObservableProperty.h>

struct TestObservablePropertyFromThis{
    void test_1(){
        class Box : public reactive::ObservablePropertyFromThis<Box>{
            int m_progress = 0;

        public:
            int progress() const{
                return m_progress;
            }

            std::future<void> load(){
                return std::async(std::launch::async, [&](){
                    while(m_progress < 1) {
                        std::cout << "Iterate" << std::endl;
                        {
                            auto lock = property_from_this()->write_lock();
                            m_progress++;
                        }
                        std::cout << "Iterate2" << std::endl;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });
            }

            void showProgress() const{
                auto lock = property_from_this()->lock();
                std::cout << "progress " << m_progress << "%" << std::endl;
            }
        };

        reactive::ObservableProperty<Box, reactive::nonblocking> box;
        /*box += [](const Box& box){
            std::cout << "Change " << std::endl;
            //box.showProgress();
        };*/
        std::cout << "load" << std::endl;
        box->load()/*.get()*/;

        std::cout << "End" << std::endl;
    }

    void test_all(){
        test_1();
    }
};

#endif //TEST_TESTOBSERVABLEPROPERTYFROMTHIS_H