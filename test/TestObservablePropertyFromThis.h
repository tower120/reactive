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

			void reset_progress() {
				auto lock = property_from_this()->write_lock();
				m_progress = 0;
			}

            auto load(){
                return std::async(std::launch::async, [&](){
                    while(m_progress < 100) {
                        {
                            auto lock = property_from_this()->write_lock();
                            m_progress++;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                });
            }

            void showProgress() const{
                auto lock = property_from_this()->lock();
                std::cout << "progress " << m_progress << "%" << std::endl;
            }
        };

        reactive::ObservableProperty<Box> box;

        box += [](const Box& box){
			if (box.progress() % 10 != 0) {
				return;
			}
            box.showProgress();
			// box.schedule()
        };
        box->load().get();

        std::cout << "End" << std::endl;
    }

    void test_all(){
        test_1();
    }
};

#endif //TEST_TESTOBSERVABLEPROPERTYFROMTHIS_H