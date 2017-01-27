# Uncomment following two lines for debugging
#OLD_SHELL := $(SHELL)
#SHELL = $(warning [$@ ($^) ($?)])$(OLD_SHELL)

CXXFLAGS = -O0 -ggdb --std=c++14
DEPFLAGS = -MMD -MP

.PHONY: all clean distclean test bench

# set default target to rp
all: rp

# handle make flags
-include .lastmakeflags

ifdef SORTED
CXXFLAGS += -DRP_SORTED
endif

ifdef USER_CONVS
CXXFLAGS += -DRP_USER_CONVS
endif

ifdef DEBUG
CXXFLAGS += -DRP_DEBUG
endif

ifeq "${LAST_SORTED};${LAST_USER_CONVS};${LAST_DEBUG}" "${SORTED};${USER_CONVS};${DEBUG}"
.lastmakeflags:
	@touch .lastmakeflags
else
.lastmakeflags: clean
	@echo "LAST_SORTED=${SORTED}" > .lastmakeflags
	@echo "LAST_USER_CONVS=${USER_CONVS}" >> .lastmakeflags
	@echo "LAST_DEBUG=${DEBUG}" >> .lastmakeflags
endif

# rewrite object generation to auto-determine dependencies, run prebuild
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)

%.o : %.c
%.o : %.c %.d .lastmakeflags
	$(COMPILE.c) $(OUTPUT_OPTION) -c %<

%.o : %.cc
%.o : %.cc %.d .lastmakeflags
	$(COMPILE.cc) $(OUTPUT_OPTION) -c $<

# system objects
OBJS = binding.o conversion.o gc.o parser.o resolver.o

rp: main.cc rp.d $(OBJS) .lastmakeflags
	$(COMPILE.cc) -o rp main.cc $(OBJS) $(LDFLAGS)

clean:
	-rm $(OBJS)
	-rm rp

distclean: clean
	-rm $(OBJS:.o=.d)
	-rm .lastmakeflags

test: rp
	@tests/run.sh

bench: rp
	@tests/bench.sh

# so make doesn't fail without dependency files
%.d: ;

# so make won't delete dependency files
.PRECIOUS: .lastmakeflags %.d

# include dependency files
-include: $(OBJS:.o=.d)
-include rp.d
