CXXFLAGS = -O0 -ggdb --std=c++11

rp: main.cc ast.h
	$(CXX) $(CXXFLAGS) -o rp main.cc $(LDFLAGS)

clean:
	-rm rp
