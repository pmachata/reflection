OPENMP = #-fopenmp
TARGETS = userefl librefl.so
COMPILEFLAGS = -Wall $(OPENMP) -g -O2 -fPIC
CXXPPFLAGS = -DUSE_EXPECT $(CXXINCLUDES)
CXXFLAGS = -std=c++0x $(COMPILEFLAGS) $(CXXPPFLAGS)
CFLAGS = -std=c89 $(COMPILEFLAGS)
LDFLAGS = $(OPENMP)

DIRS = .
ALLSOURCES = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c $(dir)/*.cc $(dir)/*.h $(dir)/*.hh)) Makefile
CCSOURCES = $(filter %.c %.cc,$(ALLSOURCES))
DEPFILES = $(foreach pat,%.c %.cc,$(patsubst $(pat),$(pat)-dep,$(filter $(pat),$(CCSOURCES))))

LDFLAGS +=

all: $(TARGETS)

userefl: LD = $(CXX)
userefl: userefl.o librefl.so

librefl.so: LDFLAGS += -shared -ldw
librefl.so: LD = $(CC)
librefl.so: refl.o refl-error.o refl-module.o refl-type.o refl-obj.o refl-method.o

-include $(DEPFILES)

%.cc-dep: %.cc
	$(CXX) $(CXXFLAGS) -MM -MT '$(<:%.cc=%.o) $@' $< > $@
%.c-dep: %.c
	$(CC) $(CFLAGS) -MM -MT '$(<:%.c=%.o) $@' $< > $@

$(TARGETS):
	$(LD) $(LDFLAGS) $^ -o $@

test-%: %.o %.cc test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DSELFTEST $(@:test-%=%.cc) $(filter-out $<,$(filter %.o,$^)) -o $@
	./$@ || (rm -f $@; exit 1)

clean:
	rm -f *.o *.*-dep $(TARGETS)

.PHONY: all clean dist
