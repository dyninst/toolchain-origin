CXX	        = g++
CXXFLAGS        = -g -Wall -std=c++0x -O2

INC = -I$(DYNINST_ROOT)/$(PLATFORM)/include -I$(GPERF)/include
LIB = -L$(DYNINST_ROOT)/$(PLATFORM)/lib -L$(LIBDWARF) -L$(GPERF)/lib
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldwarf -lelf -ldynElf -lcommon

all: binFeat printIdioms

SRC = feature.cpp\
      idiom.cpp\
      operand.cpp\
      lookup.cpp\
      binFeat.cpp\
      graphlet.cpp\
      supergraph.cpp\
      colors.cpp\
      FeatureAnalyzer.cpp\
      IdiomAnalyzer.cpp\
      GraphletAnalyzer.cpp\

PRINT_SRC = printIdioms.cpp\
            feature.cpp\
	    idiom.cpp\
	    operand.cpp\
	    lookup.cpp

OBJ = $(SRC:.cpp=.o)
PRINT_OBJ = $(PRINT_SRC:.cpp=.o)

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) -o $@ $<

binFeat: $(OBJ)
	$(CXX) $(LIB) $(DEP) -o $@ $^

printIdioms: $(PRINT_OBJ)
	$(CXX) $(LIB) $(DEP) -o $@ $^

clean:
	rm -f core core.* *.core *.o binFeat printIdioms *.so *.so.* depend
