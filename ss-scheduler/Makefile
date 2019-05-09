include $(SS_STACK)/msg.mk
prefix:=$(SS_TOOLS)


level=./
include make.config

all: directories program make_drivers

include make.rules

program: scheduler-program config-program

src/config/ssinst.h:
	make -C src/config ssinst.h

scheduler-program: src/config/ssinst.h
	+make -C src/scheduler

config-program: src/config/ssinst.h
	+make -C src/config

make_drivers: program
	+make -C drivers

install: directories install_headers install_config install_scheduler install_drivers
	

install_headers:
	${MKDIR_P} ${prefix}/include/ss-scheduler
	cp -p src/scheduler/*.h ${prefix}/include/ss-scheduler/

install_drivers: make_drivers
	${MKDIR_P} ${prefix}/bin
	cp -p drivers/ss_sched ${prefix}/bin


install_scheduler: scheduler-program
	${MKDIR_P} ${prefix}/lib
	cp -p ${build}/lib/* ${prefix}/lib

install_config: config-program
	${MKDIR_P} ${prefix}/lib
	cp -p ${build}/lib/* ${prefix}/lib
	${MKDIR_P} ${prefix}/include/ss-config
	cp -p src/config/*.h ${prefix}/include/ss-config/
	cp -rfp configs ${prefix}/
	
clean:
	make -C src/scheduler clean
	make -C src/config clean
	make -C drivers clean


