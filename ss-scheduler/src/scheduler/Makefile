include $(SS_STACK)/msg.mk
level=../../
include $(level)/make.config


SYS = $(shell sys)
CXX = g++

OPT ?= -O3
#OPT = -Og
#OPT = -O0

LEX = lex
YACC = yacc -d


SOURCES= ssdfg.cpp schedule.cpp scheduler.cpp color_mapper.cpp scheduler_sa.cpp serialize-schedule.cpp serialize-ssdfg.cpp

INCLUDE_CONFIG=$(level)/src/config/
LIB_DEST=${build}/lib
OBJ_DEST=${build}/obj


PRE_OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJ_DEST)/%,$(PRE_OBJECTS)) dfg-parser.tab.o dfg-lex.yy.o

BOOST_PATH=${boost}

all: $(LIB_DEST)/libssscheduler.a $(LIB_DEST)/libssscheduler.so

CXXFLAGS += -I$(INCLUDE_CONFIG) -L$(BOOST_PATH) -fPIC 

$(LIB_DEST)/libssscheduler.a: $(OBJECTS)
	ar crs $@ $^ 
    
$(LIB_DEST)/libssscheduler.so: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LIBS)


$(OBJ_DEST)/%.o: %.cpp dfg-parser.tab.h $(DEPDIR)/%.d
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@$(POSTCOMPILE)

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCES))))

#-------- stuff to do with dfg-parser/lexer -----------
dfg-parser.tab.o: dfg-parser.tab.c ssdfg.h
	$(CXX) $(filter-out -Wall, $(CXXFLAGS)) -g -c -o $@ $<
	@$(POSTCOMPILE)

dfg-lex.yy.o: dfg-lex.yy.c
	$(CXX) $(filter-out -Wall, $(CXXFLAGS)) -g -c -o $@ $<
	@$(POSTCOMPILE)

dfg-parser.tab.c dfg-parser.tab.h: dfg-parser.y
	$(YACC) -o dfg-parser.tab.c -v $^

dfg-lex.yy.c: dfg-parser.l dfg-parser.tab.h
	$(LEX) -o dfg-lex.yy.c $<
#----------------------------------------


.phony: clean

clean:
	-rm -Rf $(LIB_DEST)/*.so $(LIB_DEST)/*.a *.o $(OBJ_DEST)/*.o  *.d  $(OBJ_DEST)/*.d $(DEPDIR)  dfg-parser.tab.c dfg-parser.tab.o dfg-lex.yy.c dfg-lex.yy.o *.tab.*  dfg-parser *.output

include ../../make.rules

