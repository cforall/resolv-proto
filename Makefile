CXXFLAGS = -O0 -ggdb --std=c++11

HOBJS = decl.h type.h expr.h utility.h

rp: main.cc $(HOBJS)
	$(CXX) $(CXXFLAGS) -o rp main.cc $(LDFLAGS)

clean:
	-rm rp
