# Makefile - simulator suite make file

# SimpleScalar(TM) Tool Suite
# Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
# All Rights Reserved. 
# 
# THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
# YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
# 
# No portion of this work may be used by any commercial entity, or for any
# commercial purpose, without the prior, written permission of SimpleScalar,
# LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
# as described below.
# 
# 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
# or implied. The user of the program accepts full responsibility for the
# application of the program and the use of any results.
# 
# 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
# downloaded, compiled, executed, copied, and modified solely for nonprofit,
# educational, noncommercial research, and noncommercial scholarship
# purposes provided that this notice in its entirety accompanies all copies.
# Copies of the modified software can be delivered to persons who use it
# solely for nonprofit, educational, noncommercial research, and
# noncommercial scholarship purposes provided that this notice in its
# entirety accompanies all copies.
# 
# 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
# PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
# 
# 4. No nonprofit user may place any restrictions on the use of this software,
# including as modified by the user, by any other authorized user.
# 
# 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
# in compiled or executable form as set forth in Section 2, provided that
# either: (A) it is accompanied by the corresponding machine-readable source
# code, or (B) it is accompanied by a written offer, with no time limit, to
# give anyone a machine-readable copy of the corresponding source code in
# return for reimbursement of the cost of distribution. This written offer
# must permit verbatim duplication by anyone, or (C) it is distributed by
# someone who received only the executable form, and is accompanied by a
# copy of the written offer of source code.
# 
# 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
# currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
# 2395 Timbercrest Court, Ann Arbor, MI 48105.
# 
# Copyright (C) 2000-2002 by The Regents of The University of Michigan.
# Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.

##################################################################
#
# Modify the following definitions to suit your build environment,
# NOTE: most platforms should not require any changes
#
##################################################################

#
# Define below C compiler and flags, machine-specific flags and libraries,
# build tools and file extensions, these are specific to a host environment,
# pre-tested environments follow...
#

##
## vanilla Unix, GCC build
##
## NOTE: the SimpleScalar simulators must be compiled with an ANSI C
## compatible compiler.
##
## tested hosts:
##
##	Slackware Linux version 2.0.33, GNU GCC version 2.7.2.2
##	FreeBSD version 3.0-current, GNU egcs version 2.91.50
##	Alpha OSF1 version 4.0, GNU GCC version 2.7.2
##	PA-RISC HPUX version B.10.01, GNU GCC version 2.7-96q3
##	SPARC SunOS version 5.5.1, GNU egcs-2.90.29
##	RS/6000 AIX Unix version 4, GNU GCC version cygnus-2.7-96q4
##	Windows NT version 4.0, Cygnus CygWin/32 beta 19
##
CC = g++
#OFLAGS = -O0 -g -m32 -DMIN_SYSCALL_MODE -DUSE_SSE_MOVE -Wall -DDEBUG # turn off static if using valgrind
#OFLAGS = -O3 -g -m32 -DMIN_SYSCALL_MODE -DUSE_SSE_MOVE -Wall -static -fomit-frame-pointer -fexpensive-optimizations -mtune=pentium4 -march=pentium4 -msse2 -mno-sse3 -mfpmath=sse -malign-double -funroll-loops
OFLAGS = -O3 -g -pg -m32 -DMIN_SYSCALL_MODE -DUSE_SSE_MOVE -Wall -static -fexpensive-optimizations -mtune=pentium4 -march=pentium4 -msse2 -mno-sse3 -mfpmath=sse -malign-double -funroll-loops
MFLAGS = `./sysprobe -flags`
MLIBS  = `./sysprobe -libs` -lm
ENDIAN = `./sysprobe -s`
MAKE = make
AR = ar qcv
AROPT =
RANLIB = ranlib
RM = rm -f
RMDIR = rm -f
LN = ln -s
LNDIR = ln -s
DIFF = diff
OEXT = o
LEXT = a
EEXT =
CS = ;
X=/

# Compilation-specific feature flags
#
# -DDEBUG	- turns on debugging features
# -DBFD_LOADER	- use libbfd.a to load programs (also required BINUTILS_INC
#		  and BINUTILS_LIB to be defined, see below)
# -DGZIP_PATH	- specifies path to GZIP executable, only needed if SYSPROBE
#		  cannot locate binary
# -DSLOW_SHIFTS	- emulate all shift operations, only used for testing as
#		  sysprobe will auto-detect if host can use fast shifts
#
FFLAGS = -DXXX -m32

# Point the Makefile to your Simplescalar-based bunutils, these definitions
# should indicate where the include and library directories reside.
# NOTE: these definitions are only required if BFD_LOADER is defined.
#
#BINUTILS_INC = -I../include
#BINUTILS_LIB = -L../lib

##################################################################
#
# YOU SHOULD NOT NEED TO MODIFY ANYTHING BELOW THIS COMMENT
#
##################################################################

#
# complete flags
#
CFLAGS = $(MFLAGS) $(FFLAGS) $(OFLAGS) $(BINUTILS_INC) $(BINUTILS_LIB)

#
# all the sources
#
SRCS =  \
bbtracker.c          eio.c                endian.c              eval.c             \
loader.c 		         machine.c            main.c                memory.c           \
misc.c               options.c            range.c               regs.c             \
sim-eio.c            sim-fast.c           stats.c               symbol.c           \
syscall.c 					 sysprobe.c           sim-cache.c                              \
target-x86/loader.c  target-x86/symbol.c  target-x86/syscall.c  target-x86/x86.c

HDRS = \
bbtracker.h          cache.h                                    thread.h              \
eio.h                endian.h             eval.h                eventq.h           \
host.h               loader.h             machine.h             memory.h           \
mem-system.h         misc.h               options.h             ptrace.h           \
range.h              regs.h               resource.h            sim.h              \
stats.h              symbol.h             syscall.h             version.h          \
target-x86/x86.def   target-x86/x86.h     target-x86/elf.h   \
target-x86/x86flow.def

OBJS_NOMAIN =	\
eio.$(OEXT)          endian.$(OEXT)       eval.$(OEXT)          loader.$(OEXT)     \
machine.$(OEXT)      memory.$(OEXT)       misc.$(OEXT)          options.$(OEXT)    \
range.$(OEXT)        regs.$(OEXT)         stats.$(OEXT)         symbol.$(OEXT)

OBJS = main.$(OEXT) $(OBJS_NOMAIN) syscall.$(OEXT)

# Zesto specific files
ZSRCS = sim-zesto.cpp zesto-core.cpp zesto-opts.cpp zesto-oracle.cpp zesto-fetch.cpp zesto-decode.cpp \
        zesto-alloc.cpp zesto-exec.cpp zesto-commit.cpp zesto-dump.c zesto-cache.cpp                \
        zesto-dram.cpp zesto-bpred.cpp zesto-memdep.cpp zesto-prefetch.cpp zesto-uncore.cpp         \
        zesto-MC.cpp

ZHDRS = zesto-structs.h zesto-core.h zesto-opts.h zesto-oracle.h zesto-fetch.h zesto-decode.h  \
        zesto-alloc.h zesto-exec.h zesto-commit.h zesto-dump.h zesto-cache.h                   \
        zesto-dram.h zesto-bpred.h zesto-memdep.h zesto-prefetch.h zesto-uncore.h              \
        zesto-MC.h

ZOBJS = zesto-opts.$(OEXT) zesto-core.$(OEXT) zesto-oracle.$(OEXT) zesto-fetch.$(OEXT) zesto-decode.$(OEXT)                   \
        zesto-alloc.$(OEXT) zesto-exec.$(OEXT) zesto-commit.$(OEXT) zesto-dump.$(OEXT) zesto-cache.$(OEXT) \
        zesto-dram.$(OEXT) zesto-bpred.$(OEXT) zesto-memdep.$(OEXT) zesto-prefetch.$(OEXT)                 \
        zesto-uncore.$(OEXT) zesto-MC.$(OEXT)

EXOOBJS = libexo/libexo.$(OEXT) libexo/exolex.$(OEXT)



#
# programs to build
#

include make.target

#
# all targets, NOTE: library ordering is important...
#
#all: $(PROGS)
default: sim-zesto
all: sim-zesto sim-bpred sim-eio sim-fast sim-uop sim-cache main

syscall.$(OEXT): syscall.c syscall.h thread.h
	gcc $(CFLAGS) -c $*.c

make.target:
	touch make.target

config-x86: clean
	-$(RM) machine.h machine.c machine.def 
	-$(RM) loader.c symbol.c syscall.c make.target
	$(LN) target-x86$(X)x86.h machine.h
	$(LN) target-x86$(X)x86.c machine.c
	$(LN) target-x86$(X)x86.def machine.def
	$(LN) target-x86$(X)loader.c loader.c
	$(LN) target-x86$(X)symbol.c symbol.c
	$(LN) target-x86$(X)syscall.c syscall.c
	$(LN) target-x86$(X)make.x86 make.target
	-$(RMDIR) tests
	$(LNDIR) tests-x86 tests

sysprobe$(EEXT):	sysprobe.c
	$(CC) $(FFLAGS) -o sysprobe$(EEXT) sysprobe.c
	@echo endian probe results: $(ENDIAN)
	@echo probe flags: $(MFLAGS)
	@echo probe libs: $(MLIBS)
	-$(RM) tests$(X)sysprobe$(EEXT)
	$(LN) ../sysprobe$(EEXT) tests$(X)sysprobe$(EEXT)
	-$(RM) libexo$(X)sysprobe$(EEXT)
	$(LN) ../sysprobe$(EEXT) libexo$(X)sysprobe$(EEXT)

sim-fast$(EEXT):	sysprobe$(EEXT) sim-fast.$(OEXT) $(OBJS) $(EXOOBJS) bbtracker.$(OEXT)
	$(CC) -o sim-fast$(EEXT) $(CFLAGS) sim-fast.$(OEXT) $(OBJS) bbtracker.$(OEXT) $(EXOOBJS) $(MLIBS)

sim-fast2$(EEXT):	sysprobe$(EEXT) sim-fast.$(OEXT) $(OBJS) $(EXOOBJS) bbtracker.$(OEXT)
	$(CC) -o sim-fast2$(EEXT) $(CFLAGS) sim-fast.$(OEXT) $(OBJS) bbtracker.$(OEXT) $(EXOOBJS) $(MLIBS)

sim-uop$(EEXT):	sysprobe$(EEXT) sim-uop.$(OEXT) $(OBJS) $(EXOOBJS)
	$(CC) -o sim-uop$(EEXT) $(CFLAGS) sim-uop.$(OEXT) $(OBJS) $(EXOOBJS) $(MLIBS)

sim-eio$(EEXT):	sysprobe$(EEXT) sim-eio.$(OEXT) $(OBJS) $(EXOOBJS)
	$(CC) -o sim-eio$(EEXT) $(CFLAGS) sim-eio.$(OEXT) $(OBJS) $(EXOOBJS) $(MLIBS)

sim-cache$(EEXT):	sysprobe$(EEXT) sim-cache.$(OEXT) $(OBJS) $(EXOOBJS) zesto-cache.$(OEXT) zesto-core.$(OEXT)
	$(CC) -o sim-cache$(EEXT) $(CFLAGS) sim-cache.$(OEXT) $(OBJS) zesto-cache.$(OEXT) zesto-core.$(OEXT) $(EXOOBJS) $(MLIBS)


sim-bpred$(EEXT):	sysprobe$(EEXT) sim-bpred.$(OEXT) $(OBJS) $(EXOOBJS) zesto-bpred.$(OEXT)
	$(CC) -o sim-bpred$(EEXT) $(CFLAGS) sim-bpred.$(OEXT) $(OBJS) $(EXOOBJS) zesto-bpred.$(OEXT) $(MLIBS)

sim-bpred2$(EEXT):	sysprobe$(EEXT) sim-bpred.$(OEXT) $(OBJS) $(EXOOBJS) zesto-bpred.$(OEXT)
	$(CC) -o sim-bpred2$(EEXT) $(CFLAGS) sim-bpred.$(OEXT) $(OBJS) $(EXOOBJS) zesto-bpred.$(OEXT) $(MLIBS)

sim-zesto$(EEXT):	sysprobe$(EEXT) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS)
	$(CC) -o sim-zesto$(EEXT) $(CFLAGS) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS) $(MLIBS)
sim-zesto2$(EEXT):	sysprobe$(EEXT) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS)
	$(CC) -o sim-zesto2$(EEXT) $(CFLAGS) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS) $(MLIBS)

sim-zesto3$(EEXT):	sysprobe$(EEXT) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS)
	$(CC) -o sim-zesto3$(EEXT) $(CFLAGS) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS) $(MLIBS)

sim-zesto4$(EEXT):	sysprobe$(EEXT) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS)
	$(CC) -o sim-zesto4$(EEXT) $(CFLAGS) sim-zesto.$(OEXT) $(OBJS) $(ZOBJS) $(EXOOBJS) $(MLIBS)


	
exo $(EXOOBJS): sysprobe$(EEXT)
	cd libexo $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "AR=$(AR)" "AROPT=$(AROPT)" "RANLIB=$(RANLIB)" "CFLAGS=$(MFLAGS) $(FFLAGS) $(OFLAGS)" "OEXT=$(OEXT)" "LEXT=$(LEXT)" "EEXT=$(EEXT)" "X=$(X)" "RM=$(RM)" libexo.$(LEXT)



# The various *.list files are automatically generated from
# directory listings of the respective source directories.
zesto-fetch.$(OEXT): zesto-fetch.cpp zesto-fetch.h ZPIPE-fetch
	perl make_def_lists.pl fetch
	$(CC) $(CFLAGS) -c $*.cpp

zesto-decode.$(OEXT): zesto-decode.cpp zesto-decode.h ZPIPE-decode
	perl make_def_lists.pl decode
	$(CC) $(CFLAGS) -c $*.cpp

zesto-alloc.$(OEXT): zesto-alloc.cpp zesto-alloc.h ZPIPE-alloc
	perl make_def_lists.pl alloc
	$(CC) $(CFLAGS) -c $*.cpp

zesto-exec.$(OEXT): zesto-exec.cpp zesto-exec.h ZPIPE-exec
	perl make_def_lists.pl exec
	$(CC) $(CFLAGS) -c $*.cpp

zesto-commit.$(OEXT): zesto-commit.cpp zesto-commit.h ZPIPE-commit
	perl make_def_lists.pl commit
	$(CC) $(CFLAGS) -c $*.cpp

zesto-bpred.$(OEXT): zesto-bpred.cpp zesto-bpred.h ZCOMPS-bpred ZCOMPS-fusion ZCOMPS-btb ZCOMPS-ras
	perl make_def_lists.pl bpred
	$(CC) $(CFLAGS) -c $*.cpp

zesto-memdep.$(OEXT): zesto-memdep.cpp zesto-memdep.h ZCOMPS-memdep
	perl make_def_lists.pl memdep
	$(CC) $(CFLAGS) -c $*.cpp

zesto-prefetch.$(OEXT): zesto-prefetch.cpp zesto-prefetch.h ZCOMPS-prefetch
	perl make_def_lists.pl prefetch
	$(CC) $(CFLAGS) -c $*.cpp

zesto-dram.$(OEXT): zesto-dram.cpp zesto-dram.h ZCOMPS-dram
	perl make_def_lists.pl dram
	$(CC) $(CFLAGS) -c $*.cpp

zesto-MC.$(OEXT): zesto-MC.cpp zesto-MC.h ZCOMPS-MC
	perl make_def_lists.pl MC
	$(CC) $(CFLAGS) -c $*.cpp

.c.$(OEXT):
	$(CC) $(CFLAGS) -c $*.c

.cpp.$(OEXT):
	$(CC) $(CFLAGS) -c $*.cpp

filelist:
	@echo $(SRCS) $(HDRS) Makefile

sim-tests: sysprobe$(EEXT)
sim-tests: test-eio test-uop

test-eio: sim-eio$(EEXT)
	cd tests $(CS) \
	$(MAKE) "MAKE=$(MAKE)" "RM=$(RM)" "ENDIAN=$(ENDIAN)" tests \
		"DIFF=$(DIFF)" "SIM_DIR=.." "SIM_BIN=sim-eio$(EEXT)" \
		"X=$(X)" "CS=$(CS)" $(CS) \
	cd ..

# removed from clean's RM list
#$(PROGS)

clean:
	-$(RM) *.o *.obj core *~ Makefile.bak sysprobe$(EEXT)
	cd libexo $(CS) $(MAKE) "RM=$(RM)" "CS=$(CS)" clean $(CS) cd ..

depend: 
	makedepend/makedepend -n $(BINUTILS_INC) $(SRCS) $(ZSRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bbtracker.o: /usr/include/stdlib.h /usr/include/stdio.h /usr/include/malloc.h
bbtracker.o: bbtracker.h
eio.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
eio.o: memory.h stats.h eval.h loader.h thread.h libexo/libexo.h host.h misc.h
eio.o: machine.h syscall.h sim.h endian.h eio.h
endian.o: endian.h thread.h machine.h host.h misc.h machine.def zesto-structs.h
endian.o: regs.h options.h memory.h stats.h eval.h loader.h
eval.o: host.h misc.h eval.h machine.h machine.def zesto-structs.h regs.h
eval.o: options.h
loader.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
loader.o: options.h endian.h thread.h memory.h stats.h eval.h sim.h eio.h
loader.o: loader.h target-x86/elf.h target-x86/x86.h
machine.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
machine.o: options.h eval.h memory.h stats.h sim.h thread.h
machine.o: target-x86/x86flow.def
main.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
main.o: endian.h thread.h memory.h stats.h eval.h version.h loader.h sim.h
memory.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
memory.o: options.h stats.h eval.h memory.h
misc.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
options.o: host.h misc.h options.h
range.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
range.o: symbol.h loader.h memory.h stats.h eval.h thread.h range.h
regs.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
regs.o: loader.h memory.h stats.h eval.h thread.h
sim-eio.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
sim-eio.o: options.h memory.h stats.h eval.h loader.h thread.h syscall.h eio.h
sim-eio.o: range.h sim.h
sim-fast.o: host.h misc.h thread.h machine.h machine.def zesto-structs.h regs.h
sim-fast.o: options.h memory.h stats.h eval.h loader.h syscall.h sim.h
sim-fast.o: bbtracker.h
stats.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
stats.o: eval.h stats.h
symbol.o: host.h misc.h loader.h machine.h machine.def zesto-structs.h regs.h
symbol.o: options.h memory.h stats.h eval.h thread.h symbol.h target-x86/elf.h
symbol.o: target-x86/x86.h
syscall.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h regs.h
syscall.o: options.h memory.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.o: syscall.h
sysprobe.o: host.h misc.h endian.c endian.h thread.h machine.h machine.def
sysprobe.o: zesto-structs.h regs.h options.h memory.h stats.h eval.h loader.h
loader.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
loader.o: options.h endian.h thread.h memory.h stats.h eval.h sim.h eio.h
loader.o: loader.h target-x86/elf.h target-x86/x86.h
symbol.o: host.h misc.h loader.h machine.h machine.def zesto-structs.h regs.h
symbol.o: options.h memory.h stats.h eval.h thread.h symbol.h target-x86/elf.h
symbol.o: target-x86/x86.h
syscall.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h regs.h
syscall.o: options.h memory.h stats.h eval.h loader.h sim.h endian.h eio.h
syscall.o: syscall.h
x86.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h options.h
x86.o: eval.h memory.h stats.h sim.h thread.h target-x86/x86flow.def
sim-zesto.o: host.h misc.h machine.h machine.def zesto-structs.h regs.h
sim-zesto.o: options.h memory.h stats.h eval.h loader.h thread.h syscall.h sim.h
sim-zesto.o: zesto-opts.h zesto-core.h zesto-oracle.h zesto-fetch.h
sim-zesto.o: zesto-decode.h zesto-bpred.h zesto-alloc.h zesto-exec.h
sim-zesto.o: zesto-commit.h zesto-dram.h zesto-cache.h zesto-uncore.h
sim-zesto.o: zesto-MC.h
zesto-core.o: zesto-core.h zesto-structs.h machine.h host.h misc.h
zesto-core.o: machine.def regs.h options.h
zesto-opts.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-opts.o: regs.h options.h memory.h stats.h eval.h loader.h zesto-opts.h
zesto-opts.o: zesto-core.h zesto-oracle.h zesto-fetch.h zesto-decode.h
zesto-opts.o: zesto-alloc.h zesto-exec.h zesto-cache.h zesto-commit.h
zesto-opts.o: zesto-dram.h zesto-uncore.h zesto-MC.h
zesto-oracle.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-oracle.o: regs.h options.h memory.h stats.h eval.h syscall.h loader.h
zesto-oracle.o: zesto-core.h zesto-opts.h zesto-oracle.h zesto-fetch.h
zesto-oracle.o: zesto-bpred.h zesto-decode.h zesto-alloc.h zesto-exec.h
zesto-oracle.o: zesto-commit.h zesto-dram.h zesto-cache.h
zesto-fetch.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-fetch.o: regs.h options.h memory.h stats.h eval.h zesto-core.h
zesto-fetch.o: zesto-opts.h zesto-oracle.h zesto-fetch.h zesto-alloc.h
zesto-fetch.o: zesto-cache.h zesto-decode.h zesto-prefetch.h zesto-bpred.h
zesto-fetch.o: zesto-exec.h zesto-commit.h
zesto-decode.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-decode.o: regs.h options.h memory.h stats.h eval.h zesto-core.h
zesto-decode.o: zesto-opts.h zesto-oracle.h zesto-decode.h zesto-fetch.h
zesto-decode.o: zesto-bpred.h
zesto-alloc.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-alloc.o: regs.h options.h memory.h stats.h eval.h zesto-core.h
zesto-alloc.o: zesto-opts.h zesto-oracle.h zesto-fetch.h zesto-decode.h
zesto-alloc.o: zesto-alloc.h zesto-exec.h zesto-commit.h
zesto-exec.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-exec.o: regs.h options.h memory.h stats.h eval.h zesto-core.h
zesto-exec.o: zesto-opts.h zesto-oracle.h zesto-fetch.h zesto-bpred.h
zesto-exec.o: zesto-decode.h zesto-alloc.h zesto-exec.h zesto-commit.h
zesto-exec.o: zesto-cache.h zesto-memdep.h zesto-prefetch.h zesto-dram.h
zesto-exec.o: zesto-uncore.h zesto-MC.h
zesto-commit.o: sim.h options.h stats.h host.h machine.h misc.h machine.def
zesto-commit.o: zesto-structs.h regs.h eval.h memory.h thread.h zesto-core.h
zesto-commit.o: zesto-opts.h zesto-oracle.h zesto-fetch.h zesto-decode.h
zesto-commit.o: zesto-alloc.h zesto-exec.h zesto-cache.h zesto-commit.h
zesto-commit.o: zesto-bpred.h zesto-memdep.h
zesto-dump.o: zesto-dump.h thread.h machine.h host.h misc.h machine.def
zesto-dump.o: zesto-structs.h regs.h options.h memory.h stats.h eval.h
zesto-cache.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-cache.o: regs.h options.h memory.h stats.h eval.h zesto-core.h
zesto-cache.o: zesto-opts.h zesto-cache.h zesto-prefetch.h zesto-dram.h
zesto-cache.o: zesto-uncore.h zesto-MC.h
zesto-dram.o: thread.h machine.h host.h misc.h machine.def zesto-structs.h
zesto-dram.o: regs.h options.h memory.h stats.h eval.h zesto-opts.h
zesto-dram.o: zesto-cache.h zesto-dram.h zesto-uncore.h zesto-MC.h
zesto-bpred.o: sim.h options.h stats.h host.h machine.h misc.h machine.def
zesto-bpred.o: zesto-structs.h regs.h eval.h memory.h thread.h valcheck.h
zesto-bpred.o: zesto-core.h zesto-bpred.h
zesto-memdep.o: sim.h options.h stats.h host.h machine.h misc.h machine.def
zesto-memdep.o: zesto-structs.h regs.h eval.h memory.h thread.h valcheck.h
zesto-memdep.o: zesto-opts.h zesto-core.h zesto-memdep.h
zesto-prefetch.o: sim.h options.h stats.h host.h machine.h misc.h machine.def
zesto-prefetch.o: zesto-structs.h regs.h eval.h memory.h thread.h valcheck.h
zesto-prefetch.o: zesto-opts.h zesto-core.h zesto-bpred.h zesto-cache.h
zesto-prefetch.o: zesto-prefetch.h
