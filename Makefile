.PHONY: default build copy
default: build copy

CXX ?= g++
build:
	${CXX} -std=c++11 -Wall Answer.cpp

copy:
	cat Answer.cpp | xclip -sel clip
