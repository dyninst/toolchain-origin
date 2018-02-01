CXX	        = g++
CXXFLAGS        = -g -Wall -std=c++0x -O2 -D_GLIBCXX_USE_CXX11_ABI=0 

DYNINST_DIR = /p/paradyn/development/xmeng/dyninstapi/dyninst_development/x86_64-unknown-linux2.4

INC = -I$(DYNINST_DIR)/include
LIB = -L$(DYNINST_DIR)/lib
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -ldynDwarf -ldw -ldynElf -lelf -lcommon

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
