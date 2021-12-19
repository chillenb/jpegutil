include make.defs

all: jpegutil

%.o: %.cc
	$(CXX) $(CXXVERFLAGS) $(CXXFLAGS) $(CXXINFOFLAGS) $(CXXOPTFLAGS) -c $< -o $@

jpegutil: main.o jpegutil.o
	$(CXX) $(CXXFLAGS) $(LIBS) $^ -o $@

clean:
	rm -rf *.o jpegutil

.PHONY: clean
