#include "FeatureAnalyzer.h"
#include "feature.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

void IdiomAnalyzer::ProduceAFunction(InstanceDataType* idt) {
    ParseAPI::Function *f = idt->f;
	FeatureVector fv;
	fv.eval(featSize,f,true,false);
	
	for(auto fvit=fv.begin() ; fvit != fv.end(); ++fvit) {
	    const string& f = (*fvit)->format();
        idt->featPair[f] += 1;
	}  
}

