#include <cstring>
#include <string>
#include <iostream>
#include <sstream>

#include "config.h"

using namespace std;

Config::Config() {
    featOutFile = NULL
    featSize = DEFAULT_FEATURE_SIZE;
    featType = -1;
}

int Config::ReadConfigFile(const char * path) {

    
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Cannot find config file!\n"); 
        return -1;
    }
    
    char option[1024];
    char str[1024];

    while (fscanf(f, "%s", option) != EOF){
        if (!strcmp(option, "authorship")) {
            fscanf(f, "%s", sourceAuthorshipDir);
        }
        
        if (!strcmp(option, "binarypath")) {
            fscanf(f, "%s", binPath);
        }
        
        if (!strcmp(option, "featuretype")) {
            fscanf(f, "%s", str);
	    for (int i = 0; featString[i] != NULL && strcmp(featString[i], str); ++i);
	    if (featString[i] != NULL)
	        featType = i;
        }

	if (!strcmp(option, "featuresize")) {
	    fscanf(f, "%d", featSize);
	}
        
       
        if (!strcmp(option, "debug")){
            debug = true;
        }
    }
    fclose(f);

    if (featType < 0) {
        fprintf(stderr, "Incorrect feature type!\n");
	return -1;
    }

    return 0;
}


void Config::Close(){
    fclose(binaryFile);
    if (statisticFile != NULL) fclose(statisticFile);
    if (dataSetStatFile != NULL) fclose(dataSetStatFile);
    if (funcToAuthorFile != NULL) fclose(funcToAuthorFile);
    if (sourceFuncsFile != NULL) fclose(sourceFuncsFile);
    if (idiomFeature) fclose(idiomFile);
    if (graphletFeature) fclose(graphletFile);
    if (libcallFeature) fclose(libcallFile);
    if (ngramFeature) fclose(ngramFile);
    if (dataFeature) fclose(dataFile);
    if (statFeature) fclose(statFile);
    if (featureList) fclose(featureListFile);
}
