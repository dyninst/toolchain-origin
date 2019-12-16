CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -O2 -fopenmp

DYNINST_DIR=/home/xm13/dyninst-pp/install
INC = -I$(DYNINST_DIR)/include
LIB = -L$(DYNINST_DIR)/lib
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -ldynDwarf -ldw -ldynElf -lelf -lcommon -lboost_system -lpthread -fopenmp

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
	  FeatureQueue.cpp

OBJ = $(SRC:.cpp=.o)
PRINT_OBJ = $(PRINT_SRC:.cpp=.o)

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) -o $@ $<

extractFeat: $(OBJ)
	$(CXX) $(LIB) -o $@ $^ -Wl,-rpath='$(DYNINST_DIR)/lib' $(DEP) 

clean:
	rm -f core core.* *.core *.o extractFeat
