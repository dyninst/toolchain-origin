#include <algorithm>
#include <stdio.h>
#include "FeatureAnalyzer.h"

#include "CodeSource.h"

using namespace std;

void FeatureAnalyzer::PrintFeatureList() {
    string listFile = outPrefix + ".featlist";
    FILE *f = fopen(listFile.c_str(), "w");
    vector< pair<int, string> > featList;
    for (auto fit = featureIndex.begin(); fit != featureIndex.end(); ++fit)
        featList.push_back(make_pair(fit->second, fit->first));
    sort(featList.begin(), featList.end());
    for (auto fit = featList.begin(); fit != featList.end(); ++fit)
        fprintf(f, "%d %s\n", fit->first, fit->second.c_str());
    fclose(f);
}

int FeatureAnalyzer::Setup(const char *bPath, int fSize, const char *oPre) {
    featSize = fSize;
    outPrefix = string(oPre);
    totalFeatures = 0;
    
    SymtabCodeSource *sts = new SymtabCodeSource((char *)bPath);
    if (sts == NULL) {
        fprintf(stderr, "Cannot create SymtabCodeSource for %s\n", bPath);
	return -1;
    }
    co = new CodeObject(sts);
    if (co == NULL) {
        fprintf(stderr, "Cannot create CodeObject for %s\n", bPath);
	return -1;
    }
    co->parse();
    return 0;
}

int FeatureAnalyzer::GetFeatIndex(const std::string &feat) {
    auto fit = featureIndex.find(feat);
    if (fit == featureIndex.end()) return -1;
    return fit->second;
}

void FeatureAnalyzer::AddFeat(const std::string &feat) {
    auto fit = featureIndex.find(feat);
    if (fit == featureIndex.end()) {
        ++totalFeatures;
	featureIndex.insert(make_pair(feat, totalFeatures));
    }
}

void FeatureAnalyzer::AddAndPrintFeat(const std::string &feat, double count) {
    AddFeat(feat);
    int index = GetFeatIndex(feat);
    fprintf(featFile, " %d:%.3f", index, count);
}

FeatureAnalyzer::FeatureAnalyzer() {}

void FeatureAnalyzer::Analyze() {
    fprintf(stderr, "Call FeatureAnalyzer::Analyze(). Do nothing.\n");
}

bool FeatureAnalyzer::InTextSection(Dyninst::ParseAPI::Function *f) {
    if (co->cs()->linkage().find(f->addr()) == co->cs()->linkage().end()) return true;
    return false;
}
