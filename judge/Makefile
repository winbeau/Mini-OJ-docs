CXX      = g++
CXXFLAGS = -O2 -std=c++17

judge: judge.cpp
	$(CXX) $(CXXFLAGS) -o judge judge.cpp

test: judge
	bash run_tests.sh

clean:
	rm -f judge

.PHONY: test clean
