#ifndef _SUPERGRAPH_H_
#define _SUPERGRAPH_H_

#include <vector>
#include <string>
#include <map>
#include <set>

#include "colors.h"
#include "graphlet.h"
namespace graphlets {


class snode;
class edge {
 friend class graph;
 public:
    edge(snode * A, snode * B, unsigned type)
      : src_(A),
        trg_(B),
        type_(type) 
    { }
    ~edge() {}

    snode * src() const { return src_; }
    snode * trg() const { return trg_; }
    unsigned type() const { return type_; }
 
 private:
    snode * src_;
    snode * trg_;
    unsigned type_;
};

class snode {
 friend class graph;
 public:
    snode() 
      : color_(NULL) 
    { }

    void setColor(Color * c) { color_ = c; }

    std::vector<edge*> const& ins() const { return in_; }
    std::vector<edge*> const& outs() const { return out_; }

    std::string name_;
    Color * color() const { return color_; }
 private:
    std::vector<edge*> in_;
    std::vector<edge*> out_;
    Color * color_;
};

class graph {
 public:
    graph() { }
    ~graph()
    {
        for(unsigned i=0;i<nodes_.size();++i)
            delete nodes_[i];
        for(unsigned i=0;i<edges_.size();++i)
            delete edges_[i];
    }

    snode * addNode() {
        snode * n = new snode();
        nodes_.push_back(n);
        return n;
    }

    edge * link(snode * A, snode * B, unsigned t) {
        edge * e = new edge(A,B,t);
        A->out_.push_back(e);
        B->in_.push_back(e); 
	edges_.push_back(e);
        return e;
    }

    void compact();
    void todot(int&) const;
    void todot(int&,bool string) const;

    std::vector<edge*> const& edges() const { return edges_; }
    std::vector<snode*> const& nodes() const { return nodes_; }

    void mkgraphlets(std::map<graphlet,int> & cnts,bool docolor, bool doanon);
    void mkgraphlets_new(int size, std::map<graphlet,int> & cnts,bool docolor, bool doanon);

    node edge_sets(snode *A, snode *B, snode *C, bool docolor, bool doanon);
    node edge_sets(snode *A, std::set<snode*> &nodes, bool docolor, bool doanon);

 private:
    void merge(snode const* A, snode const* B, snode * C);
    void update_edges(snode const*, snode *, edge*);
    
    std::vector<snode*> nodes_;
    std::vector<edge*> edges_;

    void enumerate_subgraphs(std::set<snode*> &notConsidered,
			     std::set<snode*> &neighbors,
			     std::set<snode*> &cur,
			     int size,
			     std::map<graphlet,int> &counts,
			     bool docolor, bool doanon);
};

}

#endif
