#include "Symtab.h"
#include "Module.h"
#include "LineInformation.h"
#include "Type.h"
#include "CFG.h"
#include "CodeSource.h"
#include "CodeObject.h"

#include "InstructionDecoder.h"
#include "Instruction.h"

#include "feature.h"
#include "graphlet.h"
#include "colors.h"
#include "supergraph.h"

#include "AuthorSet.h"
#include "config.h"
#include "authorship.h"
#include "feature_analyzer.h"
#include "types.h"
#include "statistics.h"

#include <errno.h>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>

#include <string>
#include <vector>
#include <map>
#include <set>

#include <unordered_map>

#include "gperftools/profiler.h"
#include "gperftools/heap-profiler.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::SymtabAPI;
using namespace Dyninst::InstructionAPI;
using namespace graphlets;

bool FeatureAnalyzer::NORT = true;
bool FeatureAnalyzer::NOPLT = true;
int FeatureAnalyzer::NODES = 3;
int FeatureAnalyzer::N = 3;
bool FeatureAnalyzer::ANON = false;
int FeatureAnalyzer::MERGE = 1;

FeatureAnalyzer::FeatureAnalyzer(Config *c, Authorship *a):
    config(c), authorship(a), totalBasicBlock(0), failedBasicBlock(0), totalFeatures(0) {}

int FeatureAnalyzer::Analyze(char *filename) {

    SymtabCodeSource *sts = new SymtabCodeSource(filename);
    Symtab *obj = sts->getSymtabObject();
    CodeObject *co = new CodeObject(sts);
    PrefetchLineInformation(co,sts, obj);
    Statistics stat;

    const CodeObject::funclist &funcs = co->funcs();
    for(CodeObject::funclist::iterator fit = funcs.begin(); fit != funcs.end(); ++fit){
        ParseAPI::Function * f = *fit;
        
        if (strncmp(f->name().c_str(),"std::",5) == 0 || strncmp(f->name().c_str(),"__gnu_cxx::",11) == 0) {
            //fprintf(stderr,"skipped %s\n",f->name().c_str());
            continue;
        }
        
        if (!(*fit)->blocks().empty()) {
            if (NORT && (*fit)->src() == RT)
                continue;
            
            if (NOPLT && sts->linkage().find((*fit)->addr()) != sts->linkage().end())
                continue;
            
            stat.Increment();
            
            WeightedAuthorship wa;

	    FunctionType retCode = AnalyzeFunction(f , obj, filename, wa);
	    fprintf(config->funcToAuthorFile, "%s %lx", filename, f->addr());
	    wa.print(config->funcToAuthorFile);
	    stat.Increment(retCode);
	    if (retCode == PartialAuthorSignal || retCode == PureAuthorSignal){
                fprintf(config->sourceFuncsFile, "%s\n", f->name().c_str());
	    }
	    if (retCode == PartialAuthorSignal) {
	        stat.IncrementPartial(wa.type());
	    }	
            if (retCode != PureAuthorSignal){
                continue;    
	    }	   
	    // Currently we only care about functions with 
	    // Pure authorship label.	  
	    ContributionType type = wa.type();
	    stat.Increment(type);
	    /*
	    if (type == Empty) {
	        printf("%s contains empty WA\n", f->name().c_str());
	    }
	    */
            if (type != Pure) {
	        continue;
	    }
	    ++funcCount[f->name()];
//	    printf("%d\n", funcCount[f->name()]);
	    if (funcCount[f->name()] != 1) continue;

	    int label = wa.MaxAuthor();
            if (config->idiomFeature) ProduceIdiomFeature(f, label);
            if (config->graphletFeature) ProduceGraphletFeature(f, label);
            if (config->libcallFeature) ProduceLibcallFeature(co, f, label);
            if (config->ngramFeature) ProduceNGramFeature(sts, f, label);
	    if (config->dataFeature) ProduceDataFeature(f, obj, label);
	    if (config->statFeature) ProduceStatFeature(f, label);
	    ProduceDataSetStatInfo(filename, f);

        }
    }
   
    stat.Print(config->statisticFile, filename);

    totalStat.Accumulate(stat);
    return 0;

}


FunctionType FeatureAnalyzer::AnalyzeFunction(ParseAPI::Function *f, Symtab *obj, char *binaryFileName, WeightedAuthorship &funcWA){

    set<Offset> seen;
    
    ParseAPI::Function::blocklist::iterator bit = f->blocks().begin();
    
    totalBytes = 0;
    unknownLine = 0;
    unknownLib = 0;
    failFound = 0;
   
    for (; bit != f->blocks().end(); ++bit){
        Block *b = *bit;
        if (seen.find(b->start()) != seen.end()) 
            continue;
        
        seen.insert(b->start());
        totalBytes += b->end() - b->start();
        
        assert(b != NULL);
	assert(b->region() != NULL);
	if (f->name().find("event_query") != string::npos) {
	printf("For function %s\n", f->name().c_str());
	AnalyzeBasicBlock(b, obj, funcWA, binaryFileName);
	}
        
    }
    //printf("%s:Total bytes %d; Fail found bytes %.0lf; Unknown line bytes %.0lf; Unmatch bytes %.0lf\n",f->name().c_str(), totalBytes, failFound, unknownLine, unknownLib);
    if (failFound + unknownLine + unknownLib < 1e-6)
        return PureAuthorSignal;
    else if (totalBytes > failFound + unknownLine + unknownLib + 1e-6)
        return PartialAuthorSignal;
    else if (totalBytes < failFound + 1e-6)	    
        return MappingFail; 
    else if (totalBytes < unknownLib + 1e-6)
        return UnfoundFile;
    else if (totalBytes < unknownLine + 1e-6)
        return UnfoundLine;
    else 
        return MixedFail;
}

int FeatureAnalyzer::AnalyzeBasicBlock(ParseAPI::Block* b,Symtab* obj, WeightedAuthorship &funcWA, const char* binaryFileName) {
    void * buf  = b->region()->getPtrToInstruction(b->start());
    
    if(!buf) {
        dprintf("cannot get ptr to instruction\n");
	return -1;
    }

    Address cur = b->start();
    InstructionDecoder dec((unsigned char*)buf,b->size(),b->region()->getArch());
    Instruction::Ptr insn;
    int fail = 0;
    auto ait = lower_bound(addrLineInfo.begin(), addrLineInfo.end(), make_pair((int)cur, vector<LineNoTuple*>()));
    while((insn = dec.decode())) {        	
	if (ait != addrLineInfo.end() && ait->second.size() > 0){
	    vector<LineNoTuple*> &lines = ait->second;
            // If this instruction corresponds to several lines in source code,
            // we split the contribution of authorship evenly to each line.
            
	    double share = 1.0 * insn->size() / lines.size();

            for (auto lineIt = lines.begin(); lineIt != lines.end(); ++lineIt){
	        // lineIt->first  : the full path of the file
		// lineIt->second : the line number inside the file
	        printf("\tLine info for addr %lx\n", cur);	
	        AnalyzeLineNoTuple((*lineIt)->first, (*lineIt)->second, funcWA, share, binaryFileName);                     
            } 
	} else {
            // We cannot get line number for information for this instruction.
            failFound += insn->size();
	    //printf("Line number interface failed for: %x %s\n",cur,  insn->format().c_str());
	    ++fail;

        }

        // get ready for the next instruction
        cur += insn->size();
	++ait;
    }
    ++totalBasicBlock;
    if (fail) {
        ++failedBasicBlock;
	/*
        int total = 0;
        printf("Block %x contains instructions that cannot be mapped to source liens\n", b->start());
	Address cur = b->start();
	InstructionDecoder dec((unsigned char*)buf,b->size(),b->region()->getArch());
	Instruction::Ptr insn;
	while((insn = dec.decode())) {
	    printf("%x %s\n", cur, insn->format().c_str());
	    cur += insn->size();
	    ++total;
	}
	if (total == fail) printf("All block fail\n"); else printf("Partial block fail\n");
	printf("\n");
	*/
    }

    return 0;
}
int FeatureAnalyzer::AnalyzeLineNoTuple(const char *filename, int lineNumber, WeightedAuthorship& funcWA, double share, const char* binaryFileName) {
    // Find all the possible full paths containing the filename
    vector< pair<string, FileWA*> > match;
    authorship->FindFullPath(filename, match);
    printf("\t\t%s %d\n", filename, lineNumber);
    if (match.size() == 0) {
        // This line is not from the code repository.
        // It comes from external library like libc++
        dprintf("No match for file %s\n", filename);
//	if (string(filename).find("apr")!=string::npos)
        unknownLib += share;
	missingFiles.insert(string(filename));
	if (strstr(filename, "/u/x/m/xmeng/public/gcc/gcc/config/i386") == filename) unknownLib = 0;
	if (strstr(filename, "gcc-test/objdir/") != NULL) unknownLib = 0;
	if (strstr(filename, "./") != NULL) unknownLib = 0;
	if (strstr(filename, "/u/x/m/xmeng/public/gcc/mpfr/") != NULL) unknownLib = 0;
    } else {
        // We find a match or multiple matches for the instruction.
	// In either case, we can calculate the author contribution.

        pair<string, FileWA*> bestMatch;
	if (match.size() == 1){
	    // The only file to map into
	    bestMatch = match[0];
	} else {
	    // We find multiple files matching the file name given
	    // by the line number interface.
	    if (config->debug){
	        printf("Multiple match for files: %s\n", filename);
		for (size_t i = 0; i < match.size(); ++i)
		    printf("%s\n", match[i].first.c_str());
	    }
	    pair<string, string> p =  make_pair(string(binaryFileName), string(filename));
            bestMatch = ResolveBestMatch(p, match);
  
            vector<string> candidateFiles;
	    for (auto iter = match.begin(); iter != match.end(); ++iter)
	        candidateFiles.push_back(iter->first);
	    ambiguousFileMatches.insert(make_pair(string(binaryFileName), candidateFiles));

        }
   	// After determining which file to map to, we calculate the contribution
	FileWA* fileWAAddr = bestMatch.second;
	WeightedAuthorship* waAddr;

        if (fileWAAddr->GetALine(lineNumber, waAddr) < 0 ) {
	    // Out of the source file range
	    // Could be produced by compilers
            unknownLine += share;
	    //printf("Out of range for %s at line %d for %s, matched by %s\n", filename, lineNumber, binaryFileName, bestMatch.first.c_str());
        } else {
            // find WA responsible for the line, add the share to the author
	    funcWA.Update(waAddr, share);
        }
    }
    return 0;
}

// Find the best match by minimizing best edit distance   
static int f[200][50];

static int LCS(string &a, string &b){
  memset( f,0, sizeof(f));  
  for (size_t i = 0; i < a.size(); ++i)
    for (size_t j = 0; j < b.size(); ++j){
      if (a[i] == b[j]) f[i+1][j+1] = f[i][j] + 1;
      if (f[i+1][j+1] < f[i][j+1]) f[i+1][j+1] = f[i][j+1];
      if (f[i+1][j+1] < f[i+1][j]) f[i+1][j+1] = f[i+1][j];
    }
  return f[a.size()][b.size()];
}

static pair<string, FileWA*> FindBestMatch(string &s, vector< pair<string, FileWA*> > &match){
  int max = 0;
  int maxi;
  for (size_t i = 0; i < match.size(); ++i){
    if (match[i].first.size() > max) {
      max = match[i].first.size();
      maxi = i;
    }
  }
  return match[maxi];
}

pair<string, FileWA*> FeatureAnalyzer::ResolveBestMatch(pair<string, string> &p, vector< pair<string, FileWA*> > &match) {
        return FindBestMatch(p.first, match);
}

void FeatureAnalyzer::RecordAllInternalFunction(BPatch_image *image, ParseAPI::CodeObject* co){

    internalFuncs.clear();
    vector<BPatch_function*> *funcs = image->getProcedures();
    for (auto fit = funcs->begin(); fit != funcs->end(); ++fit){
        BPatch_function *bpatch_func = *fit;
	ParseAPI::Function *parse_func = ParseAPI::convert(bpatch_func);
        if (co->cs()->linkage().find(parse_func->addr()) == co->cs()->linkage().end()){
	    vector<string> names;
	    bpatch_func->getMangledNames(names);
	    internalFuncs.insert(names.begin(), names.end());
        }
    }
}

void FeatureAnalyzer::PrintFeatureList() {
    if (!config->featureList) return;
    for (auto it = featureIndex.begin(); it != featureIndex.end(); ++it) {
        fprintf(config->featureListFile, "%d %s\n", it->second, it->first.c_str());
    }
}

void FeatureAnalyzer::PrintInfo(){

    for (auto iter = missingFiles.begin(); iter != missingFiles.end(); ++iter)
        printf("missing wa for %s\n", iter->c_str());

    PrintFeatureList();
    totalStat.Print(config->statisticFile, "total info");

}


// The child process write the updated built feature index and stat data into the write end of the pipe
void FeatureAnalyzer::Write(int fd) {
    ostringstream oss;
    boost::archive::text_oarchive ar(oss);
    
    ar & totalStat;    
    ar & funcCount;
    ar & funcSize;
    ar & totalBasicBlock;
    ar & failedBasicBlock;
    ar & missingFiles;
    ar & ambiguousFileMatches; 
    ar & totalFeatures;
    ar & featureIndex;
    string str = oss.str();
    int size = str.size();
    if (write(fd, &size, sizeof(int)) != sizeof(int)) {
        fprintf(stderr, "fail in writing the size\n");
	return;
    }
    const char* buf = str.c_str();
    int cnt = 0;
    while (cnt < size) {
        int ret = write(fd, buf + cnt, size - cnt);
	if (ret < 0) {
	    fprintf(stderr, "fail in writing the object: %s\n", strerror(errno));
	    return;
	} else if (ret == 0) {
	    fprintf(stderr, "write 0 bytes for the object\n");
	    return;
	}
	cnt += ret;
    }
}

// The parent process read the updated feature index and stat data from the read end of the pipe
// and update its own index and stat data.
void FeatureAnalyzer::Read(int fd) {
    int size;
    int cnt = 0;
    char* buf = (char*)(&size);
    while (cnt < sizeof(int)) {
        int ret = read(fd, buf + cnt, sizeof(int) - cnt);
	if (ret < 0) {
	    fprintf(stderr, "fail in reading the size: %s\n", strerror(errno));
	    return;
	} else if (ret == 0) {
	    fprintf(stderr, "read 0 bytes for the size\n");
	    return;
	}
	cnt += ret;   
    }
    buf = new char[size];
    cnt = 0;
    while (cnt < size) {
        int ret = read(fd, buf + cnt, size - cnt);
	if (ret < 0) {
	    fprintf(stderr, "fail in reading the object: %s\n", strerror(errno));
	    delete buf;
	    return;
	} else if (ret == 0) {
	    fprintf(stderr, "read 0 bytes for the object\n");
	    return;
	}
	cnt += ret;   
    }
    string str(buf, size);
    istringstream iss(str);
    boost::archive::text_iarchive ar(iss);
    
    ar & totalStat;
    ar & funcCount;
    ar & funcSize;
    ar & totalBasicBlock;
    ar & failedBasicBlock;
    ar & missingFiles;
    ar & ambiguousFileMatches;
    ar & totalFeatures;
    ar & featureIndex;

    delete buf;
}


int FeatureAnalyzer::PrefetchLineInformation(CodeObject *co, SymtabCodeSource *sts, Symtab* symtab) {
    vector<Module*> mods;
    if (!symtab->getAllModules(mods)) {
        fprintf(stderr, "There is no module in the symtab object\n");
	return -1;
    }
    addrLineInfo.clear();
    const CodeObject::funclist &funcs = co->funcs();
    for(auto fit = funcs.begin(); fit != funcs.end(); ++fit){
        ParseAPI::Function * f = *fit;
        
        if (strncmp(f->name().c_str(),"std::",5) == 0 || strncmp(f->name().c_str(),"__gnu_cxx::",11) == 0) {
            //fprintf(stderr,"skipped %s\n",f->name().c_str());
            continue;
        }
        
        if (!(*fit)->blocks().empty()) {
            if (NORT && (*fit)->src() == RT)
                continue;
            
            if (NOPLT && sts->linkage().find((*fit)->addr()) != sts->linkage().end())
                continue;
            
            set<Offset> seen;
	    for (auto bit = f->blocks().begin(); bit != f->blocks().end(); ++bit){
	        Block *b = *bit;
		if (seen.find(b->start()) != seen.end()) 
		    continue;      
                seen.insert(b->start());         
                assert(b != NULL);
	        assert(b->region() != NULL);
                void * buf  = b->region()->getPtrToInstruction(b->start());
                if(!buf) {
		     dprintf("cannot get ptr to instruction\n");
		     return -1;
		}
		Address cur = b->start();
		InstructionDecoder dec((unsigned char*)buf,b->size(),b->region()->getArch());
		Instruction::Ptr insn;
		while((insn = dec.decode())) {
		    vector<LineNoTuple> lines;
		    addrLineInfo.push_back(make_pair(cur, vector<LineNoTuple*>()));
                    cur += insn->size();
		}      
            }
        }
    }
    sort(addrLineInfo.begin(), addrLineInfo.end());
    for (auto mit = mods.begin(); mit != mods.end(); ++mit) {
        Module *mod = *mit;
	LineInformation *info = mod->getLineInformation();
	if (info == NULL) {
	    //fprintf(stderr, "Module %s has no line number information\n", mod->fullName().c_str());
	    continue;
	}
	for (auto lit = info->begin(); lit != info->end(); ++lit) {
	    const pair<Offset, Offset> addrRange = lit->first;
	    LineNoTuple *lt = (LineNoTuple*)(&(lit->second));	    
	    auto itLow = lower_bound(addrLineInfo.begin(), addrLineInfo.end(), make_pair((int)addrRange.first, vector<LineNoTuple*>()));
	    for (auto iter = itLow; iter != addrLineInfo.end() && iter->first < addrRange.second; ++iter)
	        iter->second.push_back(lt);

	}
    }
    return 0;
}

