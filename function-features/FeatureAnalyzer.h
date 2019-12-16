#ifndef FEATURE_ANALYZER_H
#define FEATURE_ANALYZER_H


#include "Symtab.h"
#include "CFG.h"
#include "CodeObject.h"

#include "BPatch.h"

#include "feature.h"
#include "FeatureQueue.h"

#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>

#define dprintf if (config->debug) printf

struct InstanceDataType {
    ParseAPI::Function* f;
    std::unordered_map<std::string, double> featPair;
};


class FeatureAnalyzer {

protected: 
    // Internal functions are not written by authors
    // but produced by compilers.
    std::set<std::string> internalFuncs;
    std::string outPrefix;
    FILE *featFile;
    int featSize;

    Dyninst::ParseAPI::CodeObject *co;
    bool InTextSection(Dyninst::ParseAPI::Function *f);

    FeatureQueue q;

    typedef unordered_map<std::string, int> FeatureIndexType;
    FeatureIndexType featureIndex;

    int GetFeatureIndex(const std::string&);
    virtual void ProduceAFunction(InstanceDataType*) = 0;
public:


    void PrintFeatureList();

    // We will create a CodeObject for the file to analyze,
    // set the wanted feature size, and the prefix of the output files
    int Setup(const char *binPath, int featSize, const char *outPrefix);
    void Analyze(); 
    FeatureAnalyzer();

    void Consume();
    void Produce();

};

class IdiomAnalyzer : public FeatureAnalyzer {

protected:
    virtual void ProduceAFunction(InstanceDataType*);

};

class GraphletAnalyzer : public FeatureAnalyzer {
    static bool ANON;
    static int MERGE;
    bool color;
public:
    GraphletAnalyzer(bool c): color(c) {}
protected:
    virtual void ProduceAFunction(InstanceDataType*);

};

#endif
