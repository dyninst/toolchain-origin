#include <dyntypes.h>
#include <CFG.h>

#include "graphlet.h"
#include "supergraph.h"

using namespace graphlets;


void
graph::update_edges(snode const* n, snode * repl, edge * e) {
    if(n == e->src()) {
        repl->out_.push_back(e);
        e->src_ = repl;
    } else {
        repl->in_.push_back(e);
        e->trg_ = repl;
    }
}

void
graph::merge(snode const* A, snode const* B, snode * C)
{
    bool AtrgB = false;
    bool BtrgA = false;
    bool self = false;

    for(unsigned i=0;i<A->ins().size();++i) {
        snode * src = A->ins()[i]->src();
        if(src == A) {
            // self loop
            self = true;
        }
        else if(src != B) {
            // somebody else -- need to update their edges
            update_edges(A,C,A->ins()[i]);
        } 
    }
    for(unsigned i=0;i<A->outs().size();++i) {
        snode * trg = A->outs()[i]->trg();
        if(trg == B) {
            AtrgB = true; // remember for loop test
        }
        else if(trg != A) {
            // somebody else
            update_edges(A,C,A->outs()[i]);
        }
    }
    for(unsigned i=0;i<B->ins().size();++i) {
        snode * src = B->ins()[i]->src();
        if(src == B) {
            // self loop
            self = true;
        }
        else if(src != A) {
            // somebody else
            update_edges(B,C,B->ins()[i]);
        }
    }
    for(unsigned i=0;i<B->outs().size();++i) {
        snode * trg = B->outs()[i]->trg();
        if(trg == A) {
            BtrgA = true;
        }
        else if(trg != B) {
            // soembody else
            update_edges(B,C,B->outs()[i]);
        }
    }
    if((AtrgB && BtrgA) || self) {
        //fprintf(stderr,"Induced self in %p,%p (%d %d %d)\n",
            //A,B,AtrgB,BtrgA,self);
        link(C,C,Dyninst::ParseAPI::DIRECT); // arbitrary type
    }

    InsnColor * tmp = new InsnColor(A->color()->toint());
    tmp->merge(B->color());
    
    C->setColor(tmp);
}

void
graph::compact()
{
    std::vector<snode*> old_nodes(nodes_);
    // XXX never shrink edges; just (sometimes) create new ones and
    // (often) retire others
    //std::vector<edge*> old_edges(edges_);

    nodes_.clear();
    //edges_.clear();

    dyn_hash_map<size_t, bool> used;


    // For every node, choose a random partner and join them into
    // a new node
    for(unsigned i=0;i<old_nodes.size();++i) 
    {
        snode * n = old_nodes[i];
        if(used.find((size_t)n) != used.end())
            continue;
    
        used[(size_t)n] = true;
    

        vector<snode *> candidates;
        for(unsigned j=0;j<n->ins().size();++j) {
            if(used.find((size_t)n->ins()[j]->src()) == used.end())
                candidates.push_back(n->ins()[j]->src());
        }
        for(unsigned j=0;j<n->outs().size();++j) {
            if(used.find((size_t)n->outs()[j]->trg()) == used.end())
                candidates.push_back(n->outs()[j]->trg());
        }

        snode * join = NULL;
        if(!candidates.empty()) {
            int r = (int)(candidates.size()*(rand()/(double)RAND_MAX));
            join = candidates[r];
        }

        snode * super;
        if(join) {
            used[(size_t)join] = true;

            // manufacture a new node having the color & edge sets of
            // the two nodes (except for those edges internal to the two)
            // if either node has a self loop then the super node has a self
            // loop; if the two nodes form a loop then the super node has
            // a self loop

            super = addNode();
            used[(size_t)super] = true;
            merge(n,join,super);

            super->name_ = n->name_ + "." + join->name_;

        } else {
            // just copy
            super = n;
            nodes_.push_back(super);
            old_nodes[i] = NULL;
        }
    }

    // cleanup
    for(unsigned i=0;i<old_nodes.size();++i)
        delete old_nodes[i];
    //for(unsigned i=0;i<old_edges.size();++i)
        //delete old_edges[i];
}

void
graph::todot(int & nid) const
{
    todot(nid,false);
}

void
graph::todot(int & nid, bool as_str) const
{
    dyn_hash_map<size_t,int> nmap;
    for(unsigned i=0;i<nodes_.size();++i) {
        snode * n = nodes_[i];

        if(nmap.find((size_t)n) == nmap.end())
            nmap[(size_t)n] = nid++;

        if(as_str)
            printf("n%d [label=\"%s\"] ;\n",nmap[(size_t)n],n->color()->tostr().c_str());
        else
            printf("n%d [label=\"%d\"] ;\n",nmap[(size_t)n],n->color()->toint());
        //printf("\"%p\" ;\n",n);
        //printf("\"%s\" ;\n",n->name_.c_str());

        for(unsigned j=0;j<n->outs().size();++j) {
            if(nmap.find((size_t)n->outs()[j]->trg()) == nmap.end())
                nmap[(size_t)n->outs()[j]->trg()] = nid++;            

            printf(" n%d -> n%d ;\n",
                nmap[(size_t)n],
                nmap[(size_t)n->outs()[j]->trg()]);
            //printf(" \"%p\" -> \"%p\" ;\n",n,n->outs()[j]->trg());
            //printf(" \"%s\" -> \"%s\" ;\n",
                //n->name_.c_str(),n->outs()[j]->trg()->name_.c_str());
        }
    } 
}



// build edge type sets for A given B and C
node 
graph::edge_sets(snode * A, snode * B, snode * C,bool docolor, bool doanon)
{
    multiset<int> ins;
    multiset<int> outs;
    multiset<int> selfs;
    unsigned short color = 0;

    int a_ins[2] = {0, 0};
    int a_outs[2] = {0, 0};

    for(unsigned i=0;i<A->ins().size();++i) {
        edge * e = A->ins()[i];
        if(e->src() == B || e->src() == C) {
            if(doanon) {
                if(e->src() == B)
                    a_ins[0]++;
                else
                    a_ins[1]++;
            } else 
                ins.insert(e->type());
        }
        else if(e->src() == A)
            selfs.insert(e->type());
    }
    for(unsigned i=0;i<A->outs().size();++i) {
        edge * e = A->outs()[i];
        if(e->trg() == B || e->trg() == C) {
            if(doanon) {
                if(e->trg() == B)
                    a_outs[0]++;
                else
                    a_outs[1]++;
            } else 
                outs.insert(e->type());
        }
    }
    Color* c = NULL;
    if(docolor) { 
        color = A->color()->toint();
	c = InsnColor::ColorLookup(color);	
    }

    if(doanon) {
        if(a_ins[0] > 0)
            ins.insert(1);
        if(a_ins[1] > 0)
            ins.insert(1);
        if(a_outs[0] > 0)
            outs.insert(1);
        if(a_outs[1] > 0)
            outs.insert(1);
    }

    return node(ins,outs,selfs,c);
}

void
graph::mkgraphlets(std::map<graphlet,int> & counts,bool docolor, bool doanon)
{
    // Foreach node in the graph
    //   for each pair of its neighboring nodes
    //     make a graphlet describing this triple & record it 
    for(unsigned i=0; i< nodes().size(); ++i) {
        snode * n = nodes()[i];

        std::set<snode*> srcs;
        std::set<snode*> trgs;

        for(unsigned j=0;j<n->ins().size();++j) {
            if(n->ins()[j]->src() != n)
                srcs.insert(n->ins()[j]->src());
        }
        for(unsigned j=0;j<n->outs().size();++j) {
            if(n->outs()[j]->trg() != n)
                trgs.insert(n->outs()[j]->trg());
        }

        // Step two: build graphlets from various pairs:
        std::set<snode*>::iterator A;
        std::set<snode*>::iterator B;

        // 1. source & source
        for(A=srcs.begin();A!=srcs.end();++A) {
            B=A;++B;
            for( ; B != srcs.end(); ++B) {
                graphlet g;
		set<snode*> nodes;
		nodes.insert(*A);
		nodes.insert(*B);
		nodes.insert(n);
                g.addNode( edge_sets(*A,nodes,docolor,doanon) );
                g.addNode( edge_sets(*B,nodes,docolor,doanon) );
                g.addNode( edge_sets(n,nodes,docolor,doanon) );
                counts[g] += 1; 
                
                //printf("1 made graphlet size %d %s\n",g.size(),g.toString().c_str());
            }
        } 
    
        // 2. trg & trg
        for(A=trgs.begin();A!=trgs.end();++A) {
            B=A;++B;
            for( ; B!=trgs.end();++B) {
                graphlet g;
		set<snode*> nodes;
		nodes.insert(*A);
		nodes.insert(*B);
		nodes.insert(n);
                g.addNode( edge_sets(*A,nodes,docolor,doanon) );
                g.addNode( edge_sets(*B,nodes,docolor,doanon) );
                g.addNode( edge_sets(n,nodes,docolor,doanon) );
		counts[g] += 1; 
                //printf("2 made graphlet size %d %s\n",g.size(),g.toString().c_str());
            }
        }
                
        // 3. source & trg
        for(A=srcs.begin();A!=srcs.end();++A) {
            for(B=trgs.begin();B!=trgs.end();++B) {
                if(*A == *B)
                    continue;
                graphlet g;
		set<snode*> nodes;
		nodes.insert(*A);
		nodes.insert(*B);
		nodes.insert(n);
                g.addNode( edge_sets(*A,nodes,docolor,doanon) );
                g.addNode( edge_sets(*B,nodes,docolor,doanon) );
                g.addNode( edge_sets(n,nodes,docolor,doanon) );
                counts[g] += 1; 
                //printf("3 made graphlet size %d %s\n",g.size(),g.toString().c_str());
            }
        }
    }
}

void graph::enumerate_subgraphs(std::set<snode*> &notConsidered,
				std::set<snode*> &neighbors,
				std::set<snode*> &cur,
				int size,
				std::map<graphlet, int> &counts,
				bool docolor, bool doanon) 
{
    if ((int)cur.size() == size) {
        graphlet g;
	for (auto nit = cur.begin(); nit != cur.end(); ++nit) {
	    snode * cur_node = *nit;
	    g.addNode(edge_sets(cur_node, cur, docolor,doanon) );
	}
	++counts[g];
	return;
    }
    if ((int)cur.size() + (int)notConsidered.size() < size) return;

    snode* chosen = NULL;
    if (cur.size() == 0)
        chosen = *(notConsidered.begin());
    else {
        for (auto nit = notConsidered.begin(); nit != notConsidered.end(); ++nit)
	    if (neighbors.find(*nit) != neighbors.end()) {
	        chosen = *nit;
		break;
	    }
    }

    if (chosen == NULL) return;

    notConsidered.erase(chosen);
    enumerate_subgraphs(notConsidered, neighbors, cur, size, counts, docolor, doanon);
    cur.insert(chosen);
    set<snode*> newNeighbors(neighbors.begin(), neighbors.end());
    for (auto eit = chosen->ins().begin(); eit != chosen->ins().end(); ++eit)
        newNeighbors.insert((*eit)->src());
    for (auto eit = chosen->outs().begin(); eit != chosen->outs().end(); ++eit)
        newNeighbors.insert((*eit)->trg());
    enumerate_subgraphs(notConsidered, newNeighbors, cur, size, counts, docolor, doanon);
    cur.erase(chosen);
    notConsidered.insert(chosen);
}


void graph::mkgraphlets_new(int size, std::map<graphlet,int> & counts,bool docolor, bool doanon)
{
    if (size > 3) {
	set<snode*> cur, neighbors, notConsidered;
	notConsidered.insert(nodes_.begin(), nodes_.end());
        if (notConsidered.size() > 5000) return;
	enumerate_subgraphs(notConsidered, neighbors, cur, size, counts, docolor, doanon);
    } else if (size == 3) {
        mkgraphlets(counts,docolor,doanon);
    } else if (size == 2) {
        for(unsigned i=0; i< nodes().size(); ++i) {
	    snode * n = nodes()[i];
            std::set<snode*> trgs;
	    for(unsigned j=0;j<n->outs().size();++j) {
	        if(n->outs()[j]->trg() != n)
		    trgs.insert(n->outs()[j]->trg());
	    }
	    
	    std::set<snode*>::iterator A;
	    for(A=trgs.begin();A!=trgs.end();++A) {
	        graphlet g;
		set<snode*> nodes;
		nodes.insert(*A);
		nodes.insert(n);
                g.addNode( edge_sets(*A, nodes, docolor, doanon) );
                g.addNode( edge_sets(n, nodes, docolor, doanon) );
                counts[g] += 1; 
            }
        } 
    } else if (size == 1) {
        for(unsigned i=0; i< nodes().size(); ++i) {
	    snode * n = nodes()[i];
	    graphlet g;
	    set<snode*> nodes;
	    nodes.insert(n);
	    g.addNode( edge_sets(n, nodes, docolor,doanon) );
            counts[g] += 1; 
        } 

    }

    
}

node graph::edge_sets(snode * A, set<snode*> &nodes, bool docolor, bool doanon)
{
    multiset<int> ins;
    multiset<int> outs;
    multiset<int> selfs;

    // Currently ignore doanon parameter
    for(unsigned i=0;i<A->ins().size();++i) {
        edge * e = A->ins()[i];
	if (e->src() == A)
	    selfs.insert(e->type());
	else if (nodes.find(e->src()) != nodes.end())
	    ins.insert(e->type());
    }
    for(unsigned i=0;i<A->outs().size();++i) {
        edge * e = A->outs()[i];
	if (e->trg() != A && nodes.find(e->trg()) != nodes.end())
	    outs.insert(e->type());
    }

    if(docolor)
        return node(ins,outs, selfs, A->color());
    else
        return node(ins,outs, selfs, NULL);
}

