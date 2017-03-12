#include "FeatureAnalyzer.h"
#include "feature.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

void IdiomAnalyzer::Analyze() {
    featFile = fopen(string(outPrefix + ".instances").c_str(), "w");
    for (auto fit = co->funcs().begin(); fit != co->funcs().end(); ++fit) {
        Function *f = *fit;
	if (!InTextSection(f)) continue;
	fprintf(featFile, "%lx", f->addr());
	
	map<string, int> counts;
	FeatureVector fv;
	fv.eval(featSize,f,true,false);
	
	int cnt = 0;
	for(auto fvit=fv.begin() ; fvit != fv.end(); ++fvit) {
	    const string& f = (*fvit)->format();
	    ++counts[f];
	    ++cnt;
	}  
	for(auto cit = counts.begin() ; cit != counts.end(); ++cit) {
	    AddAndPrintFeat(cit->first, cit->second);
	}
	fprintf(featFile, "\n");
    }
    fclose(featFile);
}

