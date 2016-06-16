CXXFLAGS = -O0 -ggdb --std=c++11

OBJS = conversion.o

HOBJS = decl.h type.h expr.h utility.h

rp: main.cc $(OBJS) $(HOBJS)
	$(CXX) $(CXXFLAGS) -o rp main.cc $(OBJS) $(LDFLAGS)

clean:
	-rm rp
