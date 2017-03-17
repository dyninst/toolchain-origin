#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <stdlib.h>
#include "FeatureAnalyzer.h"

#include "Symtab.h"
#include "Region.h"

typedef enum {
    Idiom,
    Graphlet,
    UNKNOWN
} FeatureType;

map<string, FeatureType> featureMap;


using namespace std;
using namespace Dyninst::SymtabAPI;

void InitFeatureMap() {
    featureMap[string("idiom")] = Idiom;
    featureMap[string("graphlet")] = Graphlet;
}

void PrintCodeRange(char *bPath, char *oPre) {
    Symtab *obj;
    if (!Symtab::openFile(obj, string(bPath))) {
        fprintf(stderr, "Cannot open %s\n", bPath);
	exit(1);
    }
    Region* textReg;
    if (!obj->findRegion(textReg, ".text")) {
        fprintf(stderr, "Cannot find .text section\n");
	exit(1);
    }
    FILE *f = fopen(oPre, "w");
    fprintf(f, "%lx\n", textReg->getDiskOffset() + textReg->getDiskSize());
    fclose(f);
}


int main(int argc, char **argv){
    
    InitFeatureMap();
   
    if (argc != 5){
        fprintf(stderr, "Usage: extractFeat <binPath> <feature type> <feature size> <out file path>\n");
        exit(1);
    }

    int featSize = atoi(argv[3]);
    if (featSize == 0) {
        fprintf(stderr, "Wrong feature size!\n");
	exit(1);
    }

    FeatureAnalyzer *featAnalyzer = NULL;    
    auto fit = featureMap.find(string(argv[2]));
    FeatureType f = FeatureType::UNKNOWN;
    if (featureMap.find(string(argv[2])) != featureMap.end()) {
        f = fit -> second;
    }
    

    switch (f) {
        case Idiom:
	    featAnalyzer = new IdiomAnalyzer();
	    break;
	case Graphlet:
	    featAnalyzer = new GraphletAnalyzer(true);
	    break;
	default: 
	    PrintCodeRange(argv[1], argv[4]);
	    return 0;
    }

    featAnalyzer->Setup(argv[1], featSize, argv[4]);
    featAnalyzer->Analyze();
    featAnalyzer->PrintFeatureList();

}
