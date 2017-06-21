#ifndef TEST_TESTBINDABLEPROPERTY_H
#define TEST_TESTBINDABLEPROPERTY_H

#include <memory>

#include <reactive/bind.h>

#include <reactive/ObservableProperty.h>

class TestBindableProperty {
public:
    void test_simple(){
        reactive::ObservableProperty<int> len = 2;
		reactive::ObservableProperty<int> len2;
		reactive::ObservableProperty<int> y{100};

        class Box{
            int m_len = 0;
        public:

            auto len(int x) {
                m_len = x;
            }

            void show(){
                std::cout << m_len << std::endl;
            }
        };

        std::shared_ptr<Box> box = std::make_shared<Box>();


        len = 40;


		reactive::bind(len2.shared_ptr(), [](reactive::ObservableProperty<int> len2, int len, int y) {
			len2 = 2+y+len;
		}, len, y);

        auto unbind = reactive::bind(box, [](auto box, int len){
            box->len(len);
        }, len);

        len = 20;

        unbind();

        len = 30;
        box->show();


		std::cout << len2.getCopy() << std::endl;
    }


	void test_w_unsubscribe() {
		reactive::ObservableProperty<int> len = 2;
		reactive::ObservableProperty<int> y{ 100 };

		class Box {
			int m_len = 0;
		public:

			auto len(int x) {
				m_len = x;
			}

			void show() {
				std::cout << m_len << std::endl;
			}
		};

		std::shared_ptr<Box> box = std::make_shared<Box>();


		len = 40;


		reactive::bind_w_unsubscribe(box, [](auto unsubscibe, auto box, int len) {
			if (len > 100) unsubscibe();
			box->len(len);
			box->show();
		}, len);

		len = 50;
		len = 101;
		len = 60;

	}

    void test_all(){
        //test_simple();
		test_w_unsubscribe();
    }
};

#endif //TEST_TESTBINDABLEPROPERTY_H
