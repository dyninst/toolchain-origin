#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "BPatch.h"

class CallGraph {

    int totalCalls;
    int totalNodes;
    // map a function name to its integer index
    std::map<std::string, int> index;
    // use the node index to store its indegree, outdegree, and authorLabel
    std::vector<int> indegree, outdegree;    
    std::vector<int> authorLabel;
    std::set<std::pair<int,int> > relation;

public:
    void ReadAuthorFuncFile(const char* filename);
    int Analyze(char *filename);
    void AddAnEdge(int caller, int callee);
    int addIndex(std::string&);
    int getIndex(char *filename, BPatch_function *f);
    static BPatch bpatch;

    void PrintInfo();

    static void Init();
};

#endif
