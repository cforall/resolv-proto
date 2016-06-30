CXXFLAGS = -O0 -ggdb --std=c++11

.PHONY: clean prebuild

-include .lastmakeflags

ifdef SORTED
CXXFLAGS += -DRP_SORTED
endif

OBJS = conversion.o

HOBJS = decl.h flat_map.h func_table.h type.h expr.h utility.h

rp: prebuild main.cc $(OBJS) $(HOBJS)
	$(CXX) $(CXXFLAGS) -o rp main.cc $(OBJS) $(LDFLAGS)

conversion.o: prebuild conversion.cc conversion.h cost.h type.h utility.h
	$(CXX) $(CXXFLAGS) -c conversion.cc $(LDFLAGS)	

clean:
	-rm conversion.o
	-rm rp

ifeq ($(LAST_SORTED),$(SORTED))
prebuild: 
else
prebuild: clean
	@echo "LAST_SORTED=${SORTED}" > .lastmakeflags
	
endif
