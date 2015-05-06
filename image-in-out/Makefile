ESDK=$(EPIPHANY_HOME)
ELIBS=$(ESDK)/tools/host/lib
EINCS=$(ESDK)/tools/host/include
ELDF=$(ESDK)/bsps/current/internal.ldf
EXES=host epiphany.srec
OBJS=epiphany.elf
CXXFLAGS=-O2 -I $(EINCS)
CXXFLAGS=-O0 -ggdb -I $(EINCS)
CXXFLAGS=-O3 -I $(EINCS)

all: $(EXES)

host: host.o
	gcc $(CXXFLAGS) -gdb host.o -o host -L $(ELIBS) -le-hal -lrt -lIL

host.o: host.cpp shared_data.h

epiphany.elf: epiphany.c shared_data.h
	echo e-gcc -O3 -funroll-loops -ffast-math -T $(ELDF) epiphany.c -o epiphany.elf -le-lib
	e-gcc -O0 -g -T $(ELDF) epiphany.c -o epiphany.elf -le-lib

epiphany.srec: epiphany.elf
	e-objcopy --srec-forceS3 --output-target srec epiphany.elf epiphany.srec

clean:
	rm $(EXES) $(OBJS)
