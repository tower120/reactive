// flag for VS2017 c++ compiler
#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include <iostream>

#include "TestEvent.h"
#include "TestObservableProperty.h"
#include "TestBindableProperty.h"
#include "TestReactiveProperty.h"
#include "TestMultiObserver.h"


#include "BenchmarkOwnedProperty.h"
#include "BenchmarkReactivity.h"
#include "BenchmarkDeferredContainer.h"


int main() {
	
    //TestEvent().test_all();
    //TestObservableProperty().test_all();
	//TestMultiObserver().test_all();
	//TestReactiveProperty().test_all();
	/*
    TestBindableProperty().test_all();


	
	BenchmarkOwnedProperty().benchmark_all();
	*/
	//BenchmarkDeferredContainer().benchmark_all();

	BenchmarkReactivity().benchmark_all();
	
    return 0;
}