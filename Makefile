OPT ?= -O2
CXXFLAGS ?= $(OPT) -ggdb --std=c++14
DEPFLAGS = -MMD -MP

.PHONY: all clean distclean test bench

# set default target to rp
all: rp bench_gen

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

ifeq "${LAST_OPT};${LAST_SORTED};${LAST_USER_CONVS};${LAST_DEBUG}" "${OPT};${SORTED};${USER_CONVS};${DEBUG}"
.lastmakeflags:
	@touch .lastmakeflags
else
.lastmakeflags: clean
	@echo "LAST_OPT=${OPT}" > .lastmakeflags
	@echo "LAST_SORTED=${SORTED}" >> .lastmakeflags
	@echo "LAST_USER_CONVS=${USER_CONVS}" >> .lastmakeflags
	@echo "LAST_DEBUG=${DEBUG}" >> .lastmakeflags
endif

# set up source and build directories
SRCDIR = src
VPATH = $(SRCDIR) $(addprefix $(SRCDIR)/, ast data driver merge resolver bench_gen)
IFLAGS = -I$(SRCDIR)
BUILDDIR = build

# rewrite object generation to auto-determine dependencies, run prebuild
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(IFLAGS) $(TARGET_ARCH)
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) $(TARGET_ARCH)

$(BUILDDIR)/%.o : %.c
$(BUILDDIR)/%.o : %.c %.d .lastmakeflags
	$(COMPILE.c) $(OUTPUT_OPTION) -c $<

$(BUILDDIR)/%.o : %.cc
$(BUILDDIR)/%.o : %.cc %.d .lastmakeflags
	$(COMPILE.cc) $(OUTPUT_OPTION) -c $<

# rp objects
OBJS = $(addprefix $(BUILDDIR)/, binding.o conversion.o gc.o parser.o resolver.o)

BENCH_OBJS = $(addprefix $(BUILDDIR)/, binding.o gc.o random_partitioner.o)

rp: rp.cc rp.d $(OBJS) .lastmakeflags
	$(COMPILE.cc) -o rp $< $(OBJS) $(LDFLAGS)

bench_gen: bench_gen.cc bench_gen.d $(BENCH_OBJS)
	$(COMPILE.cc) -o bench_gen $< $(BENCH_OBJS) $(LDFLAGS)

clean:
	-rm $(OBJS)
	-rm rp

distclean: clean
	-rm $(OBJS:.o=.d)
	-rm rp.d
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
-include bench_gen.d
