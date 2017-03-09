#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <stdlib.h>
#include "FeatureAnalyzer.h"


typedef enum {
    Idiom,
    Graphlet,
    UNKNOWN
} FeatureType;

map<string, FeatureType> featureMap;


using namespace std;

void InitFeatureMap() {
    featureMap[string("idiom")] = Idiom;
    featureMap[string("graphlet")] = Graphlet;

}


int main(int argc, char **argv){
    
    InitFeatureMap();
   
    if (argc != 5){
        fprintf(stderr, "Usage: binFeat <binPath> <feature type> <feature size> <out file path>\n");
        exit(1);
    }

    int featSize = atoi(argv[3]);
    if (featSize == 0) {
        fprintf(stderr, "Wrong feature size!\n");
	exit(1);
    }

    FeatureAnalyzer *featAnalyzer = NULL;    

    if (featureMap.find(string(argv[2])) == featureMap.end()) {
	fprintf(stderr, "Unknown feature type!\n");
	exit(1);	
    }

    switch (featureMap[string(argv[2])]) {
        case Idiom:
	    featAnalyzer = new IdiomAnalyzer();
	    break;
	case Graphlet:
	    featAnalyzer = new GraphletAnalyzer(true);
	    break;
	default:
	    fprintf(stderr, "Unknown feature type!\n");
	    exit(1);
    }

    featAnalyzer->Setup(argv[1], featSize, argv[4]);
    featAnalyzer->Analyze();
    featAnalyzer->PrintFeatureList();

}
