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

void GraphletAnalyzer::ProduceAFunction(InstanceDataType* idt) {
    ParseAPI::Function *f = idt->f;
    map<graphlet,int> c1, c2;
    dyn_hash_map<Address, bool> visited;
    
    // Instruction graphlet
    graph * g = func_to_graph(f,visited, INSNSCOLOR);
    g->mkgraphlets_new(featSize, c1, color, ANON);
    delete g; 
    
    // Branch graphlets
    visited.clear();
    g = func_to_graph(f, visited, BRANCHCOLOR);
    g->mkgraphlets_new(featSize, c2, color, ANON);
    delete g;
    
    // Generate graphlet string representation
    for (auto pair : c1){
        string f = "G_" + (pair.first).compact(color);
        idt->featPair[f] += pair.second;
    }  
    
    for (auto pair : c2){
        string f = "B_" + (pair.first).compact(color);
        idt->featPair[f] += pair.second;
    }
}

