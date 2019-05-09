prefix?=$(shell pwd)/../

SYS = $(shell sys)
CXX = g++

client.out : client.cpp
	$(CXX) -g -std=c++11  -MD -o $@ $< -I${prefix}/src  -L${prefix}/lib -lsbconfig -Wl,-rpath,${prefix}/lib

clean:
	rm -rf *.o  client.out *.d
