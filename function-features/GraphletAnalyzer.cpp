#include "FeatureAnalyzer.h"
#include "supergraph.h"
#include "graphlet.h"
#include "colors.h"
#include <boost/iterator/filter_iterator.hpp>

using boost::make_filter_iterator;


using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::ParseAPI;
using namespace graphlets;

bool GraphletAnalyzer::ANON = false;
int GraphletAnalyzer::MERGE = 1;

static bool compound(Edge* e){
    static Intraproc intra_pred;
    static NoSinkPredicate nosink_pred;
    return intra_pred(e) && nosink_pred(e);
}


static graph * 
func_to_graph(ParseAPI::Function * f, dyn_hash_map<Address,bool> & seen, ColorType t)
{
    dyn_hash_map<Address,snode*> node_map;

    int nctr = 0;

    graph * g = new graph();

    dyn_hash_map<size_t,bool> done_edges;
    
    for(auto bit = f->blocks().begin() ; bit != f->blocks().end(); ++bit) {
        Block * b = *bit;
        snode * n = g->addNode();

        char nm[16];
        snprintf(nm,16,"n%d",nctr++);
        n->name_ = std::string(nm);

        node_map[b->start()] = n;
        if(COLOR) {
	    if (t == INSNSCOLOR) {
	        n->setColor(new InsnColor(node_insns_color(b)));
	    } else {
	        n->setColor(new BranchColor(node_branch_color(b)));
	    }
	}
    }
    unsigned idx = 0;
    for(auto bit = f->blocks().begin() ; bit != f->blocks().end(); ++bit) {
        Block * b = *bit;

        if(seen.find(b->start()) != seen.end())
            continue;
        seen[b->start()] = true;

        snode * n = g->nodes()[idx];
        for(auto eit = make_filter_iterator(compound, b->sources().begin(), b->sources().end()); 
	         eit != make_filter_iterator(compound, b->sources().end(), b->sources().end()); 
		 ++eit) {
            Edge * e = *eit;
            if(done_edges.find((size_t)e) != done_edges.end())
                continue;
            done_edges[(size_t)e] = true;

            if(node_map.find(e->src()->start()) != node_map.end()) {
                (void)g->link(node_map[e->src()->start()],n,e->type());
            }
        }
        for(auto eit = make_filter_iterator(compound, b->targets().begin(), b->targets().end()); 
	         eit != make_filter_iterator(compound, b->targets().end(), b->targets().end()); 
		 ++eit) {
            Edge * e = *eit;
            if(done_edges.find((size_t)e) != done_edges.end())
                continue;
            done_edges[(size_t)e] = true;

            if(node_map.find(e->trg()->start()) != node_map.end()) {
                (void)g->link(n,node_map[e->trg()->start()],e->type());
            }
        }
        ++idx;
    }
    return g;
}

const int NULLDEF = numeric_limits<int>::max();
/*
 * Calls kill other call defs
 */
static void reaching_defs(
    ParseAPI::Function *f, 
    vector<Block*> & blocks, 
    vector< set<int> > & before,
    vector< set<int> > & after,
    dyn_hash_map<void*,int> & bmap)
{
    set<int> work;
    vector<int> call_gen;


    // figure out the call generators
    call_gen.resize(blocks.size(),-1);
    for(auto cit = f->callEdges().begin() ; cit != f->callEdges().end(); ++cit) {
        int cidx = bmap[(*cit)->src()];
        call_gen[ cidx ] = cidx;
    }

    // entry block generates the "no call" definition
    Block * b = f->entry();
    int entry_id = bmap[b];
    work.insert(entry_id);

    while(!work.empty()) {

        int bidx = *(work.begin());
        work.erase(bidx);
        b = blocks[bidx];

        before[bidx].clear();
	for(auto eit = make_filter_iterator(compound, b->sources().begin(), b->sources().end()); 
	         eit != make_filter_iterator(compound, b->sources().end(), b->sources().end()); 
		 ++eit) {
	    int sidx = bmap[(*eit)->src()];
	    before[bidx].insert(after[sidx].begin(), after[sidx].end());
	}
	if (bidx == entry_id) before[bidx].insert(NULLDEF);

        set<int> newAfter;
	if (call_gen[bidx] != -1)
	    newAfter.insert(call_gen[bidx]);
	else
	    newAfter = before[bidx];

        if (newAfter != after[bidx]) {
	    after[bidx] = newAfter;
	    for(auto eit = make_filter_iterator(compound, b->targets().begin(), b->targets().end()); 
	             eit != make_filter_iterator(compound, b->targets().end(), b->targets().end()); 
		     ++eit) {
                work.insert(bmap[(*eit)->trg()]);
	    }
	}	    
    } 
}
/*
 * Construct a graph of entry, exit and call nodes, where edges indicate
 * reaching definitions
 */
static graph * collapse(
    ParseAPI::Function *f, 
    vector<Block*> & blocks, 
    vector< set<int> > & before,
    vector< set<int> > & after, 
    dyn_hash_map<void*,int> & bmap)
{
    vector<snode*> callnodes(blocks.size(),NULL);
    graph * g = new graph();
    snode * entry = g->addNode();

    // 1. Set up nodes for the call blocks
    for(auto cit = f->callEdges().begin() ; cit != f->callEdges().end(); ++cit) {
        int cidx = bmap[(*cit)->src()];
        callnodes[cidx] = g->addNode();

        // color
        std::map<Address, std::string>::iterator pltit =
            f->obj()->cs()->linkage().find((*cit)->trg()->start());
        if(pltit != f->obj()->cs()->linkage().end()) {
            LibCallColor * c = LibCallColor::ColorLookup((*pltit).second);
            callnodes[cidx]->setColor(c);
        } else {
            LocalCallColor * c = LocalCallColor::ColorLookup();
            callnodes[cidx]->setColor(c);
        }
    }
    // 2. Link the call nodes to one another
    for(unsigned i=0;i<callnodes.size();++i) {
        if(callnodes[i]) {
            set<int> & d = before[i];
            for(auto it = d.begin() ; it != d.end(); ++it) {
                if(*it == NULLDEF)
                    g->link(entry,callnodes[i],0);
                else if(*it != (int)i)
                    g->link(callnodes[*it],callnodes[i],0);
            }
        }
    }

    // 3. Find exit nodes && link according to reaching defs       
    for(unsigned i=0;i<blocks.size();++i) {
        Block * b = blocks[i];
        int tcnt = 0;
	for(auto eit = make_filter_iterator(compound, b->targets().begin(), b->targets().end());                   
	         eit != make_filter_iterator(compound, b->targets().end(), b->targets().end()); 
	         ++eit){
             ++tcnt;
        }
        if(tcnt == 0) {
            snode * exit = g->addNode();
            set<int> & d = after[i];
            for(auto it = d.begin() ; it != d.end(); ++it) {
                if(*it == NULLDEF)
                    g->link(entry,exit,0);
                else
                    g->link(callnodes[*it],exit,0);
            }
        }
    }
    return g;
}

static graph * mkcalldfa(ParseAPI::Function *f)
{
    vector<Block*> blocks;
    vector< set<int> > before, after; 
    dyn_hash_map<void*,int> bmap;

    for(auto bit = f->blocks().begin() ; bit != f->blocks().end(); ++bit) {
        bmap[*bit] = blocks.size();
        blocks.push_back(*bit);
    }
    before.resize( blocks.size() );
    after.resize( blocks.size() );

    // 1. Reaching definitions on call blocks
    reaching_defs(f,blocks,before, after, bmap);
    
    // 2. Node collapse
   return collapse(f,blocks,before, after,bmap);
    

}

static bool TooLargeBranches(graph *g) {
    if (g->nodes().size() < 50) return false;
    map<int,int> outdegreeCount;
    int topNodeCount = 0;
    int bottomNodeCount = 0;
    int exitNodeCount = 0;
    snode* specialExitNode = NULL;

    for (auto nit = g->nodes().begin(); nit != g->nodes().end(); ++nit) {
        snode *n = *nit;
	++outdegreeCount[n->outs().size()];
	if (n->outs().size() > 1) ++topNodeCount;
	else if (n->outs().size() == 1) {
	    if (n->ins().size() > 0)  ++bottomNodeCount;
	}
	else if (n->ins()[0]->src()->outs().size() == 1) ++exitNodeCount;
	else {
	    if (specialExitNode != NULL) return false;
	    specialExitNode = n;
	}

    }
    fprintf(stderr, "%d %d %d %p\n", topNodeCount, bottomNodeCount, exitNodeCount, specialExitNode);
    if (outdegreeCount.size() == 3 && outdegreeCount.find(0) != outdegreeCount.end() && outdegreeCount.find(1) != outdegreeCount.end() && bottomNodeCount == exitNodeCount)     
        
        return true;
    
    return false;
}

static graph* CollapseBranches(graph *g) {
    int topNodeCount = 0;
    int bottomNodeCount = 0;
    int exitNodeCount = 0;
    snode* specialExitNode = NULL;

    for (auto nit = g->nodes().begin(); nit != g->nodes().end(); ++nit) {
        snode *n = *nit;
	if (n->outs().size() > 1) ++topNodeCount;
	else if (n->outs().size() == 1) {
	    if (n->ins().size() > 0)  ++bottomNodeCount; 
	}
	else if (n->ins()[0]->src()->outs().size() == 1) ++exitNodeCount;
	else {
	    if (specialExitNode != NULL) fprintf(stderr, "More than one special exist nodes!\n");
	    specialExitNode = n;
	}
    }
    fprintf(stderr, "%d %d %d\n", topNodeCount, bottomNodeCount, exitNodeCount);
    if (bottomNodeCount != exitNodeCount) {
        fprintf(stderr, "bottom node count doesn't match exit node count\n");
	exit(0);
    }

    graph * newGraph = new graph();
    vector<snode*> topNodes;
    snode *entry, *bottomNode, *exitNode;

    entry = newGraph->addNode();
    topNodeCount = 1;
    for (int i = 0; i < topNodeCount; ++i)
        topNodes.push_back(newGraph->addNode());
    bottomNode = newGraph->addNode();
    exitNode = newGraph->addNode();
    specialExitNode = newGraph->addNode();

    for (int i = 0; i < topNodeCount; ++i) {
        if (i < topNodeCount - 1)
	    newGraph->link(topNodes[i], topNodes[i+1] , 0);
	else
	    newGraph->link(topNodes[i], specialExitNode, 0);
	newGraph->link(topNodes[i], bottomNode, 0);	
    }
    newGraph->link(entry, topNodes[0], 0);
    newGraph->link(bottomNode, exitNode, 0);
    return newGraph;
}

void GraphletAnalyzer::Analyze() {
//    fprintf(stderr, "Start graphlet extraction\n");
    featFile = fopen(string(outPrefix + ".instances").c_str(), "w");
    for (auto fit = co->funcs().begin(); fit != co->funcs().end(); ++fit) {
        Function *f = *fit;
	if (!InTextSection(f)) continue;
//	fprintf(stderr, "\textract from function %s\n", f->name().c_str());
	fprintf(featFile, "%lx", f->addr());
	
	map<graphlet,int> c1, c2, c3, c4;
	map<string, int> c;
	dyn_hash_map<Address, bool> visited;

	// Instruction graphlet
//	fprintf(stderr, "\t\t build insns summary graphlet graph\n");
	graph * g = func_to_graph(f,visited, INSNSCOLOR);
//	fprintf(stderr, "\t\t extract insns summary graphlet\n");
	g->mkgraphlets_new(featSize, c1, color, ANON);
/*	
	// Supergraphlet:
	// first we iteratively compress the graph
	for(int m=0;m<MERGE;++m) {
	    g->compact();
	}
        g->mkgraphlets_new(featSize, c2, color, ANON);
	delete g;
	

	// Callgraphlet
	g = mkcalldfa(f);
	
	if (TooLargeBranches(g)) {
	    graph * tmp = g;
	    g = CollapseBranches(g);
	    delete tmp;
	}
        g->mkgraphlets_new(featSize, c3, color, ANON);
*/	
	delete g; 


        // Branch graphlets
	visited.clear();
//	fprintf(stderr, "\t\t build branch graphlet graph\n");
	g = func_to_graph(f, visited, BRANCHCOLOR);
//	fprintf(stderr, "\t\t extract branch graphlet\n");
	g->mkgraphlets_new(featSize, c4, color, ANON);


	for (auto cit=c1.begin(); cit != c1.end(); ++cit){
	    string f = "G_" + (cit->first).compact(color);
	    c[f] += 1;
	}
/*	
	for (auto cit=c2.begin(); cit != c2.end(); ++cit){
	    string f = "S_" + (cit->first).compact(color);
	    c[f] += 1;
	}
	for (auto cit=c3.begin(); cit != c3.end(); ++cit){
	    string f = "C_" + (cit->first).compact(color);
	    c[f] += 1;
	}
*/	
	for (auto cit=c4.begin(); cit != c4.end(); ++cit){
	    string f = "B_" + (cit->first).compact(color);
	    c[f] += 1;
	}

	for (auto cit=c.begin(); cit != c.end(); ++cit) {
	    AddAndPrintFeat(cit->first, cit->second);
        }
        fprintf(featFile, "\n");
    }
    fclose(featFile);
}

