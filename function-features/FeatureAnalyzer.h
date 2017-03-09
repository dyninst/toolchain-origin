#ifndef FEATURE_ANALYZER_H
#define FEATURE_ANALYZER_H


#include "Symtab.h"
#include "CFG.h"
#include "CodeObject.h"

#include "BPatch.h"

#include "feature.h"

#include <set>
#include <string>
#include <map>
#include <vector>


#define dprintf if (config->debug) printf

class FeatureAnalyzer {

protected: 
    // Internal functions are not written by authors
    // but produced by compilers.
    std::set<std::string> internalFuncs;

    std::string outPrefix;

    int featSize;
    int totalFeatures;
    std::map<std::string, int> featureIndex;

    int GetFeatIndex(const std::string& feat);
    void AddFeat(const std::string& feat);
    void AddAndPrintFeat(const std::string &feat, double count);

    Dyninst::ParseAPI::CodeObject *co;
    bool InTextSection(Dyninst::ParseAPI::Function *f);
    FILE *featFile;

public:


    void PrintFeatureList();

    // We will create a CodeObject for the file to analyze,
    // set the wanted feature size, and the prefix of the output files
    int Setup(const char *binPath, int featSize, const char *outPrefix);

    
    FeatureAnalyzer();
    virtual void Analyze();

};

class IdiomAnalyzer : public FeatureAnalyzer {

public:
    virtual void Analyze();

};

class GraphletAnalyzer : public FeatureAnalyzer {
    static bool ANON;
    static int MERGE;
    bool color;
public:
    GraphletAnalyzer(bool c): color(c) {}
    virtual void Analyze();

};

#endif
