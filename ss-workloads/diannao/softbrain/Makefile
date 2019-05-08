ifndef SS_TOOLS
$(error SS_TOOLS is undefined)
endif

#CPP=/s/gcc-4.9.2/bin/g++ -fno-tree-vectorize -O3
#CPP=/s/gcc-4.7.0/bin/g++ -fno-tree-vectorize -O3
#CPP=/s/gcc-4.7.3/bin/g++ -O3


#CPP=/s/intelcompilers-2015/bin/icc -no-vec
#CPP=/s/intelcompilers-2015/bin/icc -fast -msse3 
#-qopt-report-phase=vec

CPP=riscv64-unknown-elf-g++

OPT?=-O3
CFLAGS=$(OPT) --std=c++11 -g -ggdb -gdwarf-3 
MODULE          := pooling convolution-shared convolution-private classifier conv1 conv2 conv3 conv4 pool1 pool3 pool5 class1 class3 conv1p conv2p conv3p conv4p pool1p pool3p pool5p class1p class3p conv5 conv5p
#SRC             := $(MODULE:=.cpp)
#OBJ             := $(MODULE:=.o)

.PHONY: all clean


all: $(MODULE)

#$(MODULE): $(SRC)

HEADERS=dnn.hpp

convolution-shared: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@

convolution-private: convolution.cpp  $(HEADERS)

	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@

pooling: pooling.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@

classifier: classifier.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@


CONV_DFGS=red32to1sig.dfg red16to1sig.dfg red8to1sig.dfg
CONV_DFG_HEADERS=$(CONV_DFGS:.dfg=.dfg.h)

CLASS_DFGS=red32to1sig.dfg
CLASS_DFG_HEADERS=$(CLASS_DFGS:.dfg=.dfg.h)

#pool2x2avg.dfg  test.dfg pool_simple.dfg
POOL_DFGS=pool4x4l2avg.dfg pool2x2l4avg.dfg
POOL_DFG_HEADERS=$(POOL_DFGS:.dfg=.dfg.h)

DFG_HEADERS=$(sort $(CLASS_DFG_HEADERS) $(POOL_DFG_HEADERS) $(CONV_DFG_HEADERS))

#$(info $$DFGS is [${DFGS}])
$(info $$DFG_HEADERS is [${DFG_HEADERS}])


SB_CONFIG=$(SS_TOOLS)/configs/diannao_simd64.sbmodel


$(DFG_HEADERS): %.dfg.h: %.dfg
	$(SS_TOOLS)/bin/sb_sched --max-iters=100000 --verbose $(SB_CONFIG) $< 

#conv1  Nx=500, Ny=375, Kx=9,  Ky=9,  Ni=32,  No=48,  priv=False
#conv2  Nx=200, Ny=200, Kx=18, Ky=18, Ni=8,   No=8,   priv=True 
#conv3  Nx=32,  Ny=32,  Kx=4,  Ky=4,  Ni=108, No=200, priv=False
#conv4  Nx=32,  Ny=32,  Kx=7,  Ky=7,  Ni=16,  No=512, priv=False
#conv5  Nx=256, Ny=256, Kx=11, Ky=11, Ni=256, No=384, priv=True
#
#pool1  Nx=492, Ny=367, Kx=2,  Ky=2,  Ni=12,                                
#pool3  Nx=32,  Ny=32,  Kx=4,  Ky=4,  Ni=100,                               
#pool5  Nx=256, Ny=256, Kx=2,  Ky=2,  Ni=256,                               
#
#class1                               Ni=200, No=100,                       
#class3                               Ni=960, No=20,                        

#padded versions

TF=#-DTn=16 -DTi=16 -DTii=32 -DTnn=32 -DTx=16 -DTy=16

#Tii not in convolution
conv1p: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=500 -DNy=375 -DKx=9  -DKy=9  -DNi=32  -DNn=64  -DSHARED=1      -DTnn=64 -DTn=32 -DTx=10 -DTy=25 -DTi=32

conv2p: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=200 -DNy=200 -DKx=18 -DKy=18 -DNi=16   -DNn=16  -DSHARED=0      -DTii=16 -DTi=16 -DTnn=16 -DTn=16 -DTx=4 -DTy=20

conv3p: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32  -DNy=32  -DKx=4  -DKy=4  -DNi=128 -DNn=224 -DSHARED=1      -DTi=64 -DTx=4

conv4p: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32  -DNy=32  -DKx=7  -DKy=7  -DNi=16  -DNn=512 -DSHARED=1      -DTnn=64 -DTn=64 -DTx=8

conv5p: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=8 -DNy=8 -DKx=11 -DKy=11 -DNi=256 -DNn=384 -DSHARED=0      -DTnn=64 -DTi=64 -DTx=8 -DTy=8
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=256 -DNy=256 -DKx=11 -DKy=11 -DNi=256 -DNn=384 -DSHARED=0      -DTnn=64 -DTi=64 -DTx=8 -DTy=8

pool1p: pooling.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=492 -DNy=368 -DKx=2  -DKy=2  -DNi=16     -DTii=16 -DTi=16 -DTx=2 -DTy=2
                                                                                                 
pool3p: pooling.cpp  $(HEADERS)                                                                             
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32  -DNy=32  -DKx=4  -DKy=4  -DNi=128    -DTii=64 -DTi=32 -DTx=2 -DTy=2
                                                                                                 
pool5p: pooling.cpp  $(HEADERS)                                                                             
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=256 -DNy=256 -DKx=2  -DKy=2  -DNi=256    -DTii=256 -DTi=32 -DTx=4 -DTy=4

class1p: classifier.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNi=960 -DNn=32     -DTii=192 -DTi=192 -DTx=2 -DTy=4


class3p: classifier.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNi=224 -DNn=128    -DTii=32 -DTi=32 -DTx=2 -DTy=4 


# ---------------------------------------------------------------------------------------------------------

conv1sb: convolution.cpp $(CONV_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=10  -DNy=25  -DKx=9  -DKy=9  -DNi=32  -DNn=64  -DSHARED=1      -DTnn=64 -DTn=64 -DTx=10 -DTy=25 -DTi=32 -DSB
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=500 -DNy=375 -DKx=9  -DKy=9  -DNi=32  -DNn=64  -DSHARED=1      -DTnn=64 -DTn=32 -DTx=10 -DTy=25 -DTi=32


conv2sb: convolution.cpp $(CONV_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=16 -DNy=16 -DKx=18 -DKy=18 -DNi=8   -DNn=8  -DSHARED=0      -DTii=8 -DTi=8 -DTnn=8 -DTn=8 -DTx=4 -DTy=16 -DSB
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=200 -DNy=200 -DKx=18 -DKy=18 -DNi=16   -DNn=16  -DSHARED=0      -DTii=16 -DTi=16 -DTnn=16 -DTn=16 -DTx=4 -DTy=20


conv3sb: convolution.cpp $(CONV_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=4  -DNy=4  -DKx=4  -DKy=4  -DNi=128  -DNn=64  -DSHARED=1      -DTi=32 -DTx=4 -DTy=4 -DTn=64  -DTii=32 -DTnn=64   -DSB    #Nx=32, Ny=32, Nn=224
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32 -DNy=32 -DKx=4  -DKy=4  -DNi=128 -DNn=224 -DSHARED=1      -DTi=32 -DTx=4 -DTy=4     -DSB


conv4sb: convolution.cpp $(CONV_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=8  -DNy=8  -DKx=7  -DKy=7  -DNi=16  -DNn=512 -DSHARED=1      -DTnn=64 -DTi=16 -DTn=64 -DTx=8  -DTy=8 -DTii=16   -DSB
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32  -DNy=32  -DKx=7  -DKy=7  -DNi=16  -DNn=512 -DSHARED=1      -DTnn=64 -DTn=64 -DTx=8 -DSB


conv5sb: convolution.cpp $(CONV_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=2 -DNy=2 -DKx=11 -DKy=11 -DNi=256 -DNn=64 -DSHARED=0      -DTnn=64 -DTi=32 -DTx=2 -DTy=2 -DTn=64  -DTii=32 -DTnn=64  -DSB
#	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=8 -DNy=8 -DKx=11 -DKy=11 -DNi=256 -DNn=384 -DSHARED=0      -DTnn=64 -DTi=64 -DTx=8 -DTy=8  -DSB




pool1sb: pooling.cpp  $(POOL_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=512 -DNy=368 -DKx=2  -DKy=2  -DNi=16     -DTii=16 -DTi=16 -DTx=64 -DTy=16 -DSB 

pool3sb: pooling.cpp  $(POOL_DFG_HEADERS) $(HEADERS) 
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=32  -DNy=32  -DKx=4  -DKy=4  -DNi=128    -DTii=64 -DTi=32 -DTx=16 -DTy=4 -DSB

pool5sb: pooling.cpp  $(HEADERS) $(POOL_DFG_HEADERS) 
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNx=256 -DNy=256 -DKx=2  -DKy=2  -DNi=256    -DTii=256 -DTi=32 -DTx=16 -DTy=8 -DSB 

class1sb: classifier.cpp $(CLASS_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNi=960 -DNn=20     -DTn=16 -DTnn=32 -DTii=32 -DTi=32 -DTx=2 -DTy=4 -DSB

class3sb: classifier.cpp $(CLASS_DFG_HEADERS) $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) $(TF) -static -o $@ -DNi=224 -DNn=128    -DTn=16 -DTnn=32 -DTii=32 -DTi=32 -DTx=2 -DTy=4 -DSB


# --------------------------------------------------------------------------------------------------------------------------------

conv1: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=500 -DNy=375 -DKx=9  -DKy=9  -DNi=32  -DNn=48  -DSHARED=1     -DTnn=64 -DTn=32 -DTx=10 -DTy=25 -DTi=32

conv2: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=200 -DNy=200 -DKx=18 -DKy=18 -DNi=8   -DNn=8   -DSHARED=0     -DTnn=8 -DTn=8 -DTi=8 -DTx=16 -DTy=16

conv3: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=32  -DNy=32  -DKx=4  -DKy=4  -DNi=108 -DNn=200 -DSHARED=1     -DTnn=20 -DTn=10 -DTi=20 -DTx=16 -DTy=16

conv4: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=32  -DNy=32  -DKx=7  -DKy=7  -DNi=16  -DNn=512 -DSHARED=1     -DTnn=64 -DTn=16 -DTi=16 -DTx=16 -DTy=16

conv5: convolution.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=8 -DNy=8 -DKx=11 -DKy=11 -DNi=256 -DNn=384 -DSHARED=0     -DTnn=64 -DTn=16 -DTi=16 -DTx=16 -DTy=16
#	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=256 -DNy=256 -DKx=11 -DKy=11 -DNi=256 -DNn=384 -DSHARED=0     -DTnn=64 -DTn=16 -DTi=16 -DTx=16 -DTy=16


pool1: pooling.cpp $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=492 -DNy=367 -DKx=2  -DKy=2  -DNi=12     -DTii=12 -DTi=12 -DTx=16 -DTy=16
                                                                                               
pool3: pooling.cpp $(HEADERS)                                                                  
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=32  -DNy=32  -DKx=4  -DKy=4  -DNi=100    -DTii=50 -DTi=25 -DTx=16 -DTy=16
                                                                                               
pool5: pooling.cpp $(HEADERS)                                                       
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNx=256 -DNy=256 -DKx=2  -DKy=2  -DNi=256    -DTii=64 -DTi=16 -DTx=16 -DTy=16


class1: classifier.cpp  $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNi=960 -DNn=20     -DTii=80 -DTi=20 -DTnn=20 -DTn=20


class3: classifier.cpp  $(HEADERS)
	$(CPP) $^ $(LIB) $(CFLAGS) -static -o $@ -DNi=200 -DNn=100    -DTii=40 -DTi=20 -DTnn=40 -DTn=20


clean:
	rm -f $(MODULE) convolution pooling classifier $(DFG_HEADERS)
cleansb:
	rm -f conv*sb pool*sb class*sb

ultraclean: clean cleansb
	rm -f *.log

