include $(SS_STACK)/msg.mk

level=../
include ../make.config

SYS = $(shell sys)
CXX = g++

CXXFLAGS := $(OPT) -Wall -g -std=c++11

LIB_PATH=$(SS_TOOLS)/lib 
INC_SSMODEL_PATH=$(SS_TOOLS)/include/ss-config
INC_SSSCHED_PATH=$(SS_TOOLS)/include/ss-scheduler

CXXFLAGS += -I$(INC_SSMODEL_PATH) -I$(INC_SSSCHED_PATH) -Wl,-rpath,$(LIB_PATH) 

LOCAL_LIBS = -ly -lfl -lssscheduler -lssconfig $(LIBS)

all: ss_sched 

ss_sched : ss_sched.cpp $(INC_SSMODEL_PATH)/*.h
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(LIB_PATH) $(LOCAL_LIBS)  -Wl,-rpath,${SS_TOOLS}/lib


clean:
	rm -rf *.o ss_sched *.d

include ../make.rules

