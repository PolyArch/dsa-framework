level=../../
include ${level}/make.config

SOURCES= model.cpp model_parsing.cpp fu_model.cpp sub_model.cpp direction.cpp ssinst.cpp
PRE_OBJECTS=$(SOURCES:.cpp=.o)
INST_MODEL_FILE=full.ssinst

LIB_DEST=${build}/lib
OBJ_DEST=${build}/obj

INST_HEADERS=insts64/*.h insts32/*.h insts16/*.h  insts8/*.h

PRE_OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJ_DEST)/%,$(PRE_OBJECTS))

CXXFLAGS += -fPIC

all: directories ssinst.cpp $(LIB_DEST)/libssconfig.a $(LIB_DEST)/libssconfig.so 

$(LIB_DEST)/libssconfig.a: $(OBJECTS)
	ar crs $@ $^
    
$(LIB_DEST)/libssconfig.so: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -MD -shared -o $@ $^ $(LIBS)


ssinst.cpp: ssinst.h $(INST_HEADERS)
	@echo "done"

#don't use autodeps for this, because generated files are hard
ssinst.h: inst_model.cpp inst_model.h model_parsing.cpp $(INST_HEADERS) $(INST_MODEL_FILE)
	$(CXX) $(CXXFLAGS) -MD inst_model.cpp model_parsing.cpp -o inst_model
	@$(POSTCOMPILE)
	./inst_model $(INST_MODEL_FILE) ssinst.h ssinst.cpp
    
$(OBJ_DEST)/%.o: %.cpp %.h ssinst.h $(DEPDIR)/%.d
	$(CXX) $(CXXFLAGS) -MD -c -o $@ $<
	@$(POSTCOMPILE)

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCES))))

.phony: clean

clean:
	-rm -Rf libssconfig.a *.o ssinst.cpp ssinst.h $(LIB_DEST)/*.a $(LIB_DEST)/*.so $(OBJ_DEST)/*.o *.d

include ${level}/make.rules
