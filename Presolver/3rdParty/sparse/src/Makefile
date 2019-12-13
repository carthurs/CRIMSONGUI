#  Makefile for Sparse1.4
#
#  Ken Kundert
#  kundert@users.sourceforge.net
#
#
#  To run tests ...
#  On Linux machines run: make
#  On Unix machines run: gnumake

CFLAGS = -O
LINTFLAGS = -lc -lm
SHELL = /bin/sh
CC = gcc

HFILES = spConfig.h spDefs.h spMatrix.h
CFILES = spAllocate.c spBuild.c spFactor.c spOutput.c spSolve.c spUtils.c \
	 spFortran.c
OFILES = spAllocate.o spBuild.o spFactor.o spOutput.o spSolve.o spUtils.o \
	 spFortran.o
LIBRARY = ../lib/sparse.a
DESTINATION = ../bin/sparse
TESTC = spTest.c
TESTO = spTest.o

SOURCE = $(HFILES) $(CFILES)

$(DESTINATION)	: $(LIBRARY) $(TESTO)
	$(CC) $(CFLAGS) -o $(DESTINATION) $(TESTO) $(LIBRARY) -lm

$(LIBRARY)	: $(OFILES)
	ar r   $(LIBRARY) $?
	ranlib $(LIBRARY)

spice		: $(LIBRARY) spSMP.o
	cp $(LIBRARY) ../lib/SMP.a
	ar r ../lib/SMP.a spSMP.o
	ranlib ../lib/SMP.a

dox		:
	doxygen Doxyfile

lint		:
	@lint $(LINTFLAGS) $(CFILES) $(TESTC) | grep -v "but never used"

clean		:
	rm -f $(OFILES) $(LIBRARY) $(TESTO) $(DESTINATION) spSMP.o core

touch		:
	touch -c  $(OFILES) $(LIBRARY)
	ranlib $(LIBRARY)

tags		: $(SOURCE) $(TESTC)
	ctags -t -w $(SOURCE) $(TESTC)

$(OFILES)	: $(HFILES)
$(TESTO)	: $(HFILES)
spSMP.o : spMatrix.h ../include/SMPdefs.h
