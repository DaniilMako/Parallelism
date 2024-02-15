FLAGS_DF = -std=c++17 -Wall

ifdef USE_DOUBLE
FLAGS_DF += -DUSE_DOUBLE
endif

task1_sin: task1_sin.cpp
	g++ $(FLAGS_DF) -o $@ $< -lm
