#ifndef _GRAPHLET_H_
#define _GRAPHLET_H_

#include "CodeSource.h"
#include "CodeObject.h"
#include "Function.h"
#include "dyntypes.h"
#include "colors.h"

#include <set>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

extern bool COLOR;

namespace graphlets {

/*
 * Graphlets are described by the edges of each node
 * 
 * There is a partial ordering over edges:
 *   inset > outset > selfset
 * 
 *   comparison of the edge sets is magnitude and then on
 *   edge types (edge types are ints)
 */

typedef enum {
    INSNSCOLOR,
    BRANCHCOLOR
} ColorType;


class edgeset {
 friend class node;
 public:
    edgeset() { }
    edgeset(std::multiset<int> types) :
        types_(types.begin(),types.end())
    {
    }
    edgeset(std::set<int> types) :
        types_(types.begin(),types.end())
    {
    }
    ~edgeset() { }

    void set(std::multiset<int> types) {
        types_.clear();
        types_.insert(types_.begin(),types.begin(),types.end());
    }

    bool operator<(edgeset const& o) const
    {
        if(this == &o)
            return false;

        unsigned mysz = types_.size();
        unsigned osz = o.types_.size();
       
        if(mysz != osz)
            return mysz < osz;
        else {
            if(mysz == 0) 
                return false;
            std::vector<int>::const_iterator ait = types_.begin();
            std::vector<int>::const_iterator bit = o.types_.begin();
            for( ; ait != types_.end(); ++ait, ++bit) {
                if(*ait < *bit)
                    return true;
                else if(*ait > *bit)
                    return false;
            }
            return false;
        }
        
    }

    void print() const {
        std::vector<int>::const_iterator it = types_.begin();
        for( ; it != types_.end(); ++it) {
            printf(" %d",*it);
        }
    }

    std::string toString() const {
        std::stringstream ret;
        std::vector<int>::const_iterator it = types_.begin();
        for( ; it != types_.end(); ++it) {
            ret << " " << *it;
        }
        return ret.str();
    }

    std::string compact() const {
        std::stringstream ret;
        std::map<int,int> unique;
        std::vector<int>::const_iterator it = types_.begin();
        for( ; it != types_.end(); ++it)
            unique[*it] +=1;
        std::map<int,int>::iterator uit = unique.begin();
        for( ; uit != unique.end(); ) {
            ret << (*uit).first;
            if((*uit).second > 1)
                ret << "x" << (*uit).second;
            if(++uit != unique.end())
                ret << ".";
        }
        return ret.str();
    }

 private:
    // XXX must be sorted
    std::vector<int> types_;
};

class node {
 public:
    node() : color_(NULL),colors_(false) { }
    node(std::multiset<int> & ins, std::multiset<int> & outs, std::multiset<int> &self) :
        ins_(ins), outs_(outs), self_(self),color_(NULL)
    { }
    node(std::multiset<int> & ins, std::multiset<int> & outs, std::multiset<int> &self, Color *color) :
        ins_(ins), outs_(outs), self_(self),color_(color)
    { }
    node(std::set<int> & ins, std::set<int> & outs, std::set<int> &self, Color *color) :
        ins_(ins), outs_(outs), self_(self),color_(color)
    { }

    ~node() { }

    void setIns(std::multiset<int> & ins) {
        ins_.set(ins);
    }
    void setOuts(std::multiset<int> & outs) {
        outs_.set(outs);
    }
    void setSelf(std::multiset<int> & self) {
        self_.set(self);
    }
    bool operator<(node const& o) const;
    void print() const;
    std::string toString() const;
    std::string compact(bool colors) const;


 private:
    edgeset ins_;
    edgeset outs_;
    edgeset self_;
    Color *color_;
    bool colors_;
};


class graphlet {
 public:
    graphlet() {}
    ~graphlet() {}

    void addNode(node const& node) {
        nodes_.insert(node);
    }

    bool operator<(graphlet const& o) const {
        if(this == &o)
            return false;

        unsigned asz = nodes_.size();
        unsigned bsz = o.nodes_.size();
        if(asz != bsz)
            return asz < bsz;
        else if(asz == 0)
            return false;

        std::multiset<node>::const_iterator ait = nodes_.begin();
        std::multiset<node>::const_iterator bit = o.nodes_.begin();
       
        for( ; ait != nodes_.end(); ++ait,++bit) {
            if(*ait < *bit)
                return true;
            else if(*bit < *ait)
                return false;
        }
        return false;
    }

    void print() const {
        std::multiset<node>::const_iterator it = nodes_.begin();
        for( ; it != nodes_.end(); ++it) {
            (*it).print();
            printf(" ");
        }
        printf("\n");
    }

    std::string toString() const {
        std::stringstream ret;
        std::multiset<node>::const_iterator it = nodes_.begin();
        for( ; it != nodes_.end(); ++it) {
            ret << (*it).toString() << " ";
        }
        return ret.str();
    }

    std::string compact(bool color) const {
        std::stringstream ret;
        std::multiset<node>::const_iterator it = nodes_.begin();

        for( ; it != nodes_.end(); ) {
            ret << (*it).compact(color);
            if(++it != nodes_.end())
                ret << "_";
        }
        return ret.str();
    }

    unsigned size() const { return nodes_.size(); }

 private:
    std::multiset<node> nodes_;
};


}
unsigned short node_insns_color(Block *A);
unsigned short node_branch_color(Block *A);
#endif
