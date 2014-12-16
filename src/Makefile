
OBJECTS = main.o \

CXXFLAGS=-Wall -Wextra -Wno-unknown-pragmas -Wno-import -O3 -I../include
LDFLAGS=

all: sqlsplitter

sqlsplitter: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o sqlsplitter $(OBJECTS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c -o main.o main.cpp

clean:
	$(RM) $(OBJECTS)

distclean: clean
	$(RM) sqlsplitter

.PHONY : clean distclean
