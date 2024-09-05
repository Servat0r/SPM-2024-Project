CXX               ?= g++
MPICXX			   = mpicxx
OPTFLAGS	   	   = -O3
CXXFLAGS          += -std=c++20 -Wall
INCLUDES	   	   = -I. -I./include -I./fastflow -I./cereal -I./openmpi-5.0.3
LIBS               = -pthread -latomic
SOURCES            = UTWavefrontFF.cpp
TARGET             = $(SOURCES:.cpp=)
MPI_SOURCES		   = UTWavefrontMPI.cpp
MPI_TARGET        = $(patsubst %.cpp,mpi_%, $(MPI_SOURCES))

.PHONY: help all all_ff all_mpi mpi clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

help:
	@echo "Usage: make <command> <arguments>"
	@echo "Commands: "
	@echo "- help: shows this guide"
	@echo "- all: compiles all Fastflow and MPI source files"
	@echo "- all_ff: compiles all Fastflow source files"
	@echo "- all_mpi: compiles all MPI source files"
	@echo "- clean file=<filename>: removes the specified <filename>";ù
	@echo "- cleanall: removes all executables"
	@echo "- <filename>: compiles the given file <filename>, assuming by default to be a Fastflow source with g++";
	@echo "- mpi_<filename>: compiles the given MPI source file <filename> with mpicxx"
	@echo "- mpi file=<filename>: compiles the given MPI source file <filename> with mpicxx"

all: all_ff all_mpi

all_ff: $(TARGET)

all_mpi: $(MPI_TARGET)

# Pattern rule for building MPI files with "mpi_" prefix
mpi_%: %.cpp
	$(MPICXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $* $< $(LIBS)

# Rule for compiling a specific MPI file passed as an argument
mpi:
	@if [ -n "$(file)" ]; then \
		$(MPICXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $(file) $(file).cpp $(LIBS); \
	else \
		echo "No file specified. Usage: make mpi file=<filename>"; \
	fi

# Rule for cleaning a specific file passed as an argument
clean:
	@if [ -n "$(file)" ]; then \
		rm -f $(file); \
		echo "Removed $(file)"; \
	else \
		echo "No file specified. Usage: make clean file=<filename>"; \
	fi

# Rule for cleaning all files
cleanall: clean
	-rm -fr $(TARGET) $(MPI_TARGET)
