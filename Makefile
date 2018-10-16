CXXFLAGS ?= -ggdb --std=c++14 -Wall -Wno-unused-function
DEPFLAGS = -MMD -MP
MAKEFILE_NAME = ${firstword ${MAKEFILE_LIST}}

# set up source and build directories
SRCDIR = src
VPATH = $(SRCDIR) $(addprefix $(SRCDIR)/, ast data driver merge resolver bench_gen)
IFLAGS = -I$(SRCDIR)
BUILDDIR = build

.PHONY : all clean test bench

.PRECIOUS : %.d

all : rp bench_gen

# handle make flags
-include .lastmakeflags
LAST_DIR ?= bu
LAST_RES ?= tec

# Debug Levels: #
# 0. -O0, asserts, GC_TRAP
# 1. -O0, asserts, GC_TRAP, expensive safety checks
# 2. -O0, asserts, GC_TRAP, expensive safety checks, trace printing

ifdef DBG
CXXFLAGS += -O0 -DRP_DEBUG=${DBG} -DGC_TRAP
else
CXXFLAGS += -O2 -DNDEBUG
endif

ifdef SORTED
CXXFLAGS += -DRP_SORTED
endif

ifdef USER_CONVS
CXXFLAGS += -DRP_USER_CONVS
endif

DIR ?= ${LAST_DIR}
ifeq "${DIR}" "td"
CXXFLAGS += -DRP_DIR_TD
DIR_OBJS = resolver-td.o
else ifeq "${DIR}" "co"
CXXFLAGS += -DRP_DIR_CO
DIR_OBJS = resolver-bu.o
else ifeq "${DIR}" "bu"
CXXFLAGS += -DRP_DIR_BU
DIR_OBJS = resolver-bu.o
else
$(error invalid DIR ${DIR})
endif

RES ?= ${LAST_RES}
ifeq "${RES}" "def"
CXXFLAGS += -DRP_RES_DEF
else ifeq "${RES}" "tec"
CXXFLAGS += -DRP_RES_TEC
else ifeq "${RES}" "imm"
CXXFLAGS += -DRP_RES_IMM
else ifeq "${RES}" "top"
CXXFLAGS += -DRP_RES_TOP
else
$(error invalid RES ${RES})
endif

ifeq "${LAST_DBG};${LAST_SORTED};${LAST_USER_CONVS};${LAST_DIR};${LAST_RES}" "${DBG};${SORTED};${USER_CONVS};${DIR};${RES}"
.lastmakeflags:
	@touch .lastmakeflags
else
.lastmakeflags: clean
	@echo "LAST_DBG=${DBG}" >> .lastmakeflags
	@echo "LAST_SORTED=${SORTED}" >> .lastmakeflags
	@echo "LAST_USER_CONVS=${USER_CONVS}" >> .lastmakeflags
	@echo "LAST_DIR=${DIR}" >> .lastmakeflags
	@echo "LAST_RES=${RES}" >> .lastmakeflags
endif

# rewrite object generation to auto-determine dependencies, run prebuild
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(IFLAGS) $(TARGET_ARCH)
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) $(TARGET_ARCH)

$(BUILDDIR)/%.o : %.c
$(BUILDDIR)/%.o : %.c $(BUILDDIR)/%.d .lastmakeflags
	$(COMPILE.c) $(OUTPUT_OPTION) -c $<

$(BUILDDIR)/%.o : %.cc
$(BUILDDIR)/%.o : %.cc $(BUILDDIR)/%.d .lastmakeflags
	$(COMPILE.cc) $(OUTPUT_OPTION) -c $<

# rp objects
OBJS = $(addprefix $(BUILDDIR)/, conversion.o env.o expr.o forall.o forall_substitutor.o gc.o parser.o resolver.o $(DIR_OBJS) rp.o)

# bench_gen objects
BENCH_OBJS = $(addprefix $(BUILDDIR)/, gc.o env.o expr.o forall.o forall_substitutor.o random_partitioner.o bench_gen.o)

${OBJS} ${BENCH_OBJS} : ${MAKEFILE_NAME}

rp : $(OBJS)
	$(COMPILE.cc) -o rp-$(DIR)-$(RES) $^ $(LDFLAGS)
	ln -sf rp-$(DIR)-$(RES) rp

bench_gen : $(BENCH_OBJS)
	$(COMPILE.cc) -o bench_gen $^ $(LDFLAGS)

clean :
	-rm -f $(OBJS) $(OBJS:.o=.d) rp
	-rm -f $(BENCH_OBJS) $(BENCH_OBJS:.o=.d) bench_gen
	-rm -f .lastmakeflags

test : rp
	@tests/run.sh

bench : rp
	@tests/bench.sh

# so make doesn't fail without dependency files
%.d : ;

# include dependency files
-include $(OBJS:.o=.d) $(RESOLVER_OPTS:.o=.d)
