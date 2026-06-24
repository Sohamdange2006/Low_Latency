CXX      = g++
# -O2 for a realistic speed number. -g keeps line info so cachegrind can
# point at the exact source lines that miss. -g does NOT slow -O2 codegen.
CXXFLAGS = -O2 -g -std=c++17 -Wall -Wextra

lob: main.cpp orderbook.cpp orderbook.hpp
	$(CXX) $(CXXFLAGS) main.cpp orderbook.cpp -o lob

orders.csv: gen.py
	python3 gen.py 1000000 > orders.csv

clean:
	rm -f lob orders.csv cachegrind.out.* cg.out
