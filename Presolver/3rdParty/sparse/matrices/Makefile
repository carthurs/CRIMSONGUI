#  Makefile for Sparse test suite
#
#  Ken Kundert
#  kundert@users.sourceforge.net
#
#  To run tests ...
#  On Linux machines run: make
#  On Unix machines run: gnumake
#
#  To run the tests use: make
#  To compare against archived results use: make compare
#  To update archived results use: make update
#  To remove the results files use: make clean

SPARSE = ../bin/sparse

OUT_FILES = $(patsubst %.mat,%.out,$(wildcard *.mat)) \
	    $(patsubst %.mat.gz,%.out,$(wildcard *.mat.gz))
GOLDEN_FILES = $(patsubst %.out,Archive/%.out,$(wildcard *.out))


all:	$(OUT_FILES)

$(OUT_FILES): $(SPARSE)

%.out: %.mat ; $(SPARSE) $< > $@

# On Unix machines may need to replace zcat on the following line with gzcat
%.out: %.mat.gz ; zcat $< | $(SPARSE) > $@

clean:
	rm -f $(OUT_FILES)

# The following rules are used to create and compare against
# a set of golden results files.

# update the golden results files (only do this when you
# are sure the current output files exist and are up to date)
update: $(GOLDEN_FILES)

Archive/%.out: %.out ; grep -vi time $< | grep -vi memory > $@

compare:
	@for outfile in *.out; do \
	    if grep -vi time $$outfile | grep -vi memory | cmp -s - Archive/$$outfile; then \
		echo -n; \
	    else \
		echo "diff detected in $$outfile"; \
		grep -vi time $$outfile | grep -vi memory | diff - Archive/$$outfile; \
	    fi; \
	done
