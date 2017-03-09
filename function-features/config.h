#ifndef CONFIG_H
#define CONFIG_H

#include <cstdio>
#include <string>
#include <set>

#define DEFAULT_FEATURE_SIZE 3

typedef enum {
  Single, WA
} AuthorshipType;


char* featString[] = { "idiom", 
                       "graphlet", 
		       "libcall", 
		       "ngram",
                       "metrics",
		       "data",
		       NULL}



class Config {

public:
    char binPath[1024];
    char sourceAuthorshipDir[1024];
    FILE *featOutFile;
    int featSize;
    int featType;

    AuthorshipType atype;
    std::string base;
    FILE *authorFile;


    FILE *statisticFile;
    FILE *dataSetStatFile;

    FILE *featureListFile;
    bool featureList;

    FILE *funcToAuthorFile;

    FILE *sourceFuncsFile;

    bool debug;

    Config();
    int ReadConfigFile(const char *path );
    void Close();
};

#endif 
