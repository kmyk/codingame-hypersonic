.PHONY: default build copy
default: build copy

build:
	# https://www.codingame.com/faq
	# C++: g++ 4.9.2 mode C++11 With the following libraries ‑lm, ‑lpthread, ‑ldl, ‑lcrypt; Memory limit 768MB
	g++ -std=c++11 -lm -lpthread -ldl -lcrypt -Wall -pedantic Answer.cpp

copy:
	cat Answer.cpp | xclip -sel clip
