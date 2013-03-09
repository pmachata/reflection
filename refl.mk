OPENMP = #-fopenmp
TARGETS = userefl librefl.so
COMPILEFLAGS = -Wall $(OPENMP) -g -O2 -fPIC
CXXPPFLAGS = -DUSE_EXPECT $(CXXINCLUDES)
CXXFLAGS = -std=c++0x $(COMPILEFLAGS) $(CXXPPFLAGS)
CFLAGS = -std=c99 -pedantic $(COMPILEFLAGS)
LDFLAGS = $(OPENMP)

all: $(TARGETS)

userefl: LD = $(CXX)
userefl: userefl.o librefl.so

librefl.so: LDFLAGS += -shared -ldw
librefl.so: LD = $(CC)
librefl.so: refl.o refl-error.o refl-module.o refl-type.o refl-obj.o	\
	refl-method.o refl-die.o

%.o: %.cc %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c %.d
	$(CC) $(CFLAGS) -c $< -o $@

include $(wildcard *.d)

%.d: %.cc
	$(CXX) $(CXXFLAGS) -MM -MT '$(<F:%.cc=%.o) $@' $< > $@

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT '$(<F:%.c=%.o) $@' $< > $@

$(TARGETS):
	$(LD) $(LDFLAGS) $^ -o $@

test-%: %.o %.cc test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DSELFTEST $(@:test-%=%.cc) $(filter-out $<,$(filter %.o,$^)) -o $@
	./$@ || (rm -f $@; exit 1)

clean:
	rm -f *.o *.d $(TARGETS)

distclean: clean
	rm -f Makefile

.PHONY: all clean dist distclean
.PRECIOUS: %.d
