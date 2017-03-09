#ifndef FEATURE_ANALYZER_H
#define FEATURE_ANALYZER_H

#include "config.h"
#include "authorship.h"
#include "types.h"
#include "statistics.h"

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

    Config* config;
    Authorship* authorship;
        
    // Internal functions are not written by authors
    // but produced by compilers.
    std::set<std::string> internalFuncs;
    std::vector<std::pair<int , std::vector<Dyninst::SymtabAPI::LineNoTuple*> > > addrLineInfo;
    
    // Statistics for different types of bytes in a function being analyzed
    int totalBytes;
    double unknownLine;
    double unknownLib;
    double ambiguousBytes;
    double failFound;

    // The weighted authorship of the function 
    // in terms of the bytes contributed by each author


    FunctionType AnalyzeFunction(Dyninst::ParseAPI::Function *f, Dyninst::SymtabAPI::Symtab * obj,char *binaryFileName, WeightedAuthorship& funcWA);
    int AnalyzeBasicBlock(Dyninst::ParseAPI::Block* b,Dyninst::SymtabAPI::Symtab * obj, WeightedAuthorship& funcWA, const char* binaryFileName);
    int AnalyzeLineNoTuple(const char *filename, int lineNumber, WeightedAuthorship& funcWA, double share, const char* binaryFileName);
    std::pair<std::string, FileWA*> ResolveBestMatch(std::pair<std::string, std::string> &p, std::vector< std::pair<std::string, FileWA*> > &match);

    void ProduceIdiomFeature(Dyninst::ParseAPI::Function* f, int label);
    void ProduceGraphletFeature(Dyninst::ParseAPI::Function* f, int label);
    void ProduceLibcallFeature(Dyninst::ParseAPI::CodeObject *co, Dyninst::ParseAPI::Function* f, int label);

    void NGram(int size, std::map<std::string, int> &counts, Dyninst::ParseAPI::SymtabCodeSource* sts, Dyninst::ParseAPI::Function *f); 
    void ProduceNGramFeature(Dyninst::ParseAPI::SymtabCodeSource* sts, Dyninst::ParseAPI::Function *f, int label);
    void ProduceDataFeature(Dyninst::ParseAPI::Function *f, Dyninst::SymtabAPI::Symtab * obj, int label);
    void ProduceStatFeature(Dyninst::ParseAPI::Function *f, int label);
    void ProduceDataSetStatInfo(const char* filename, Dyninst::ParseAPI::Function *f);
    void AddAFeature(FILE *f, const std::string& feature, int value);

    int PrefetchLineInformation(Dyninst::ParseAPI::CodeObject *co, Dyninst::ParseAPI::SymtabCodeSource* sts, Dyninst::SymtabAPI::Symtab *symtab);
public:

    static bool NORT;
    static bool NOPLT;
    static int NODES;
    static int N;
    static bool ANON;
    static int MERGE;

    Statistics totalStat;
    std::map<std::string, int> funcCount;
    std::map<int, int> funcSize;
   
    FeatureAnalyzer(Config *c, Authorship *a);
    int Analyze(char *fullpath);


    int totalBasicBlock;
    int failedBasicBlock;
    std::set<std::string> missingFiles;
    std::set<std::pair<std::string, std::vector<std::string> > > ambiguousFileMatches;

    int totalFeatures;
    std::map<std::string, int> featureIndex;
    void PrintFeatureList();
    void PrintInfo();
    
    void Write(int fd);
    void Read(int fd);

    Config* GetConfig() { return config; }

    static BPatch bpatch;
};
#endif
