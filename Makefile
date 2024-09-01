CXX               ?= g++
MPICXX			   = mpicxx
OPTFLAGS	   	   = -O3 -march=native
CXXFLAGS          += -std=c++20 -Wall 
INCLUDES	   	   = -I. -I./include -I./fastflow -I./cereal -I./openmpi-5.0.3 -I./sciplot -I./gnuplot-palettes
LIBS               = -pthread -latomic
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS) #-DDEBUGMODE

all: $(TARGET)

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)

mpi_%: %.cpp
	$(MPICXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $* $< $(LIBS)
