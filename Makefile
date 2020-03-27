CXX = g++
CXXFLAGS = -std=c++14 -Wall `pkg-config --cflags libndn-cxx` -g
LIBS = `pkg-config --libs libndn-cxx`
DESTDIR ?= /usr/local
SOURCE_OBJS = repo.o
PROGRAMS = repo

all: $(PROGRAMS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(LIBS)

repo: $(SOURCE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ repo.o $(LIBS)

clean:
	rm -f $(PROGRAMS) *.o

