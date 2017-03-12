CXX	        = g++
CXXFLAGS        = -g -Wall -std=c++0x -O2 -D_GLIBCXX_USE_CXX11_ABI=0 

INC = -I$(DYNINST_ROOT)/$(PLATFORM)/include -I$(GPERF)/include
LIB = -L$(DYNINST_ROOT)/$(PLATFORM)/lib -L$(LIBDWARF) -L$(GPERF)/lib -L$(ELF_LIB)
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -ldynDwarf -ldwarf -ldynElf -lelf -lcommon

all: extractFeat

SRC = feature.cpp\
      idiom.cpp\
      operand.cpp\
      lookup.cpp\
      extractFeat.cpp\
      graphlet.cpp\
      supergraph.cpp\
      colors.cpp\
      FeatureAnalyzer.cpp\
      IdiomAnalyzer.cpp\
      GraphletAnalyzer.cpp\

OBJ = $(SRC:.cpp=.o)
PRINT_OBJ = $(PRINT_SRC:.cpp=.o)

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) -o $@ $<

extractFeat: $(OBJ)
	$(CXX) $(LIB) -o $@ $^ $(DEP) 

clean:
	rm -f core core.* *.core *.o extractFeat
