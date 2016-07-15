CXXFLAGS = -O0 -ggdb --std=c++11
DEPFLAGS = -MMD -MP

.PHONY: all clean distclean prebuild

# set default target to rp
all: rp

# handle make flags
-include .lastmakeflags

ifdef SORTED
CXXFLAGS += -DRP_SORTED
endif

ifeq ($(LAST_SORTED),$(SORTED))
prebuild:
else
prebuild: clean
	@echo "LAST_SORTED=${SORTED}" > .lastmakeflags
endif

# rewrite object generation to auto-determine dependencies, run prebuild
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)

%.o : %.c
%.o : %.c %d prebuild
	$(COMPILE.c) $(OUTPUT_OPTION) -c %<

%.o : %.cc
%.o : %.cc %.d prebuild
	$(COMPILE.cc) $(OUTPUT_OPTION) -c $<

# system objects
OBJS = conversion.o parser.o resolver.o

rp: main.cc rp.d prebuild $(OBJS)
	$(COMPILE.cc) -o rp main.cc $(OBJS) $(LDFLAGS)

clean:
	-rm $(OBJS)
	-rm rp

distclean: clean
	-rm $(OBJS:.o=.d)
	-rm .lastmakeflags

# so make doesn't fail without dependency files
%.d: ;

# so make won't delete dependency files
.PRECIOUS: %.d

# include dependency files
-include: $(OBJS:.o=.d)
