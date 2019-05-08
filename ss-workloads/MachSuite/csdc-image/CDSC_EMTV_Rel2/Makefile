#Makefile
#LTE System Levle Simulator Makefile
#Author Jianwen Chen  <jwchen@ee.ucla.edu>

dbg=1
OMIT_CUTIL_LIB=1
NAME=  emtv3d 

include config.mak

ifeq ($(PLATFORM),GPU) 

# Add source files here
EXECUTABLE	:= $(NAME) 
# Cuda source files (compiled with cudacc)
CUFILES		:= Forward_Tracer_3D.cu Backward_Tracer_3D.cu  EMupdate.cu TVupdate.cu GPU_Routine.cu 
# C/C++ source files (compiled with gcc / c++)
CFILES		:= \
	Array3D.c EMTV3D.c Initialization.c Vector.c utilities.c

include common.mk

exe: $(TARGET)
	cp -rf $(TARGET) ./bin

tags: 
	ctags --langmap=c:.c.cu -R ./*

distclean: clean
	rm -f config.mak config.h

else

BINDIR= bin
INCDIR= src 
SRCDIR= src
OBJDIR= obj
LIBDIR= lib

ADDSRCDIR=
ADDINCDIR=

FLAGS= $(CFLAGS) -I$(INCDIR) -I$(ADDINCDIR) 
SUFFIX=
FLAGS+= -ffloat-store -Wall

OBJSUF= .o$(SUFFIX)

SRC=    $(wildcard $(SRCDIR)/*.c)
ADDSRC= $(wildcard $(ADDSRCDIR)/*.c)
OBJ=    $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX)) $(ADDSRC:$(ADDSRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX))
BIN=    $(BINDIR)/$(NAME)$(SUFFIX)
LIB=    $(LIBDIR)/libemtv3d.a


default: depend bin 


dependencies:
	@echo "" >dependencies

clean:
	@echo remove all objects
	@rm -rf $(OBJDIR)/*
	@rm -rf $(OBJDIR)
	@rm -f $(BIN)
	@rm -f ./*~
	@rm -f ./tags

distclean: clean
	rm -f config.mak config.h

tags:
	@echo update tag table
	@ctags -R ./*

bin:    $(OBJ)
	@echo 'creating binary "$(BIN)"'
	@$(CC) -o $(BIN) $(OBJ) $(FLAGS) $(LDFLAGS)
	@echo '... done'
	@echo

depend:
	@echo
	@echo 'checking dependencies'
	@echo 'create the obj directory'
	@mkdir -p $(OBJDIR)
	@echo 'create the bin directory'
	@mkdir -p $(BINDIR)
	@echo


$(OBJDIR)/%.o$(SUFFIX): $(SRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c -o $@ $(FLAGS) $<

$(OBJDIR)/%.o$(SUFFIX): $(ADDSRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c -o $@ $(FLAGS) $<

endif

