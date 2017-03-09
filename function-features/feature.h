/* 
 * Copyright (c) 1996-2010 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef _FEATURE_H_
#define _FEATURE_H_

#include <string>
#include <unordered_map>
#include <set>

#include "Operation.h"
#include "Register.h"
#include "CFG.h"
#include "CodeSource.h"
#define ILLEGAL_ENTRY e_No_Entry
#define NOARG 0xffff
#define IMMARG (NOARG-1)
#define MEMARG (NOARG-2)
#define MULTIREG (NOARG-3)
#define CALLEESAVEREG (NOARG-4)
#define ARGUREG (NOARG-5)
#define OTHERREG (NOARG-6)

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

enum feature_types {
    INVALID,
    IDIOM,
    OPERAND
};

class Feature {
 public:
    Feature() : _type(INVALID) { }
    feature_types type() const { return _type; }

    virtual ~Feature() { }
    virtual string format() = 0;

 protected:
    feature_types _type;
};

/* LookupFeature: 
  
   A feature that the Lookup knows how to look up.
   Composed of LookupTerms.
*/   
class LookupTerm {
 public:
    LookupTerm() { }
    LookupTerm(const LookupTerm *) { fprintf(stderr,"ZOMG BAD\n"); }
    virtual ~LookupTerm() { }
    virtual string format() = 0;

    virtual size_t hash() const = 0;
};
class LookupFeature : public Feature {
 public:
    LookupFeature() { }
    LookupFeature(vector<LookupTerm *> & t);
    const vector<LookupTerm *> & terms() const { return _terms;}
    void add_term(LookupTerm *t);

    virtual ~LookupFeature() { }
    virtual string format() = 0;


 protected:
    // this object is not responsible for the contents of this vector
    vector<LookupTerm*> _terms;
};

/* IdiomFeature:

   Represents an instruction idiom sequence.
*/
#define ENTRY_SHIFT 32ULL
#define ARG1_SHIFT 16ULL
#define ARG2_SHIFT 0ULL
#define ENTRY_SIZE 16ULL
#define ARG_SIZE 16ULL

#define ENTRY_MASK (((uint64_t)(1<<ENTRY_SIZE)-1) << ENTRY_SHIFT)
#define ARG1_MASK (((uint64_t)(1<<ARG_SIZE)-1) << ARG1_SHIFT)
#define ARG2_MASK (((uint64_t)(1<<ARG_SIZE)-1) << ARG2_SHIFT)

class IdiomTerm : public LookupTerm {
 public:
    IdiomTerm() {}
    IdiomTerm(Function *f, Address addr);
    IdiomTerm(SymtabCodeSource *sts, Address addr);
    IdiomTerm(uint64_t it);
    IdiomTerm(const IdiomTerm & it) :
        entry_id(it.entry_id),
        arg1(it.arg1),
        arg2(it.arg2),
        len(it.len)
    { }
    ~IdiomTerm() { }

    string format();
    bool operator==(const IdiomTerm &it) const;
    bool operator<(const IdiomTerm &it) const;
    
    size_t hash() const;
    uint64_t to_int() const;
    void from_int(uint64_t it);

    string human_format();

 public:
    unsigned short entry_id;
    unsigned short arg1;
    unsigned short arg2;
    unsigned char len;

};

class IdiomFeature : public LookupFeature {
 public:
    IdiomFeature() { }
    IdiomFeature(vector<LookupTerm *> & t) : LookupFeature(t)
        { }
    IdiomFeature(char * str);
    ~IdiomFeature() { }
    
    string format();

    string human_format();

    typedef IdiomTerm term;

    bool operator< (const IdiomFeature& f) const;
};

/* 
  OperandFeature

  Represents a distant bigram pair of operands
*/
class OperandTerm : public LookupTerm {
 public:
    OperandTerm(unsigned short id, bool write) :
        _id(id), _write(write), _wc_dist(0), _formatted(false) 
    { }
    OperandTerm(const OperandTerm & ot) :
        _id(ot._id),
        _write(ot._write),
        _wc_dist(ot._wc_dist),
        _formatted(ot._formatted),
        _format(ot._format)
    { }
    OperandTerm(unsigned long ot);
    ~OperandTerm() { }

    string format();
    bool operator==(const OperandTerm & ot) const;
    bool operator<(const OperandTerm & ot) const;
    size_t hash() const;

    uint64_t to_int() const;
    void from_int(uint64_t it);

 public:
    unsigned short _id;
    bool _read;
    bool _write;
    unsigned char _wc_dist;
    bool _formatted;
    string _format; 
};
class OperandFeature : public LookupFeature {
 public:    
    OperandFeature() : _formatted(false) { }
    ~OperandFeature() { }

    string format();
    
    typedef OperandTerm term;
 private:
    bool _formatted;
    string _format;
};

template<typename T>
class Lookup {
 public:
    typedef typename T::term LT;

    Lookup(bool f = false) :
        fixed(false)
    { }
    ~Lookup();

    void lookup(int size, Function * f, Address addr, Address end, vector<Feature *> & feats);
    void lookup_cross_blk(int size, Function *f, Block *b, Address addr, vector<Feature*> &feats);

    struct lt_hash {
        size_t operator()(const LT & x) const {
            return x.hash();
        }
    };
    struct lt_equal {
        bool operator()(const LT & a, const LT &b) const {
            return a == b;
        }
    };

    class Lnode;
    typedef unordered_map<LT, Lnode *, lt_hash, lt_equal> lmap_t;
    class Lnode {
     public:
        LookupFeature *f;
        lmap_t _next;

        Lnode();
        ~Lnode();

        Lnode * next(LT *nt);
    };
 private:
    void lookup_idiom(int size, Function *f, Address addr, Address end,  Lnode * cur, int depth, 
        vector<LookupTerm *> & stack, vector<Feature *> & feats);
    void lookup_idiom_cross_blk(int size, Function *f, Block *b, Address addr, Lnode * cur, int depth, 
        vector<LookupTerm *> & stack, vector<Feature *> & feats);

 private:

    unordered_map<Address, vector<LT *> > _term_map;

    Lnode start;
    bool fixed;
};


/* Evaluating a FeatureVector against a Function
   produces... a vector of Features. Each of these
   can be printed out in the appropriate format.

   There are two modes:

    1. Feature subset provided. In this case, only
       those features indicated will be produced.
       The ordering of features under iteration is fixed.

    2. All features enabled. There is no guarantee
       of feature ordering in this case.
*/
class FeatureVector {
 public:
    FeatureVector();
    FeatureVector(char * featfile);
    ~FeatureVector();

    int eval(int size, Function * f, bool idioms = true, bool operands = false);
    int eval_cross_blk(int size, Function * f, bool idioms = true, bool operands = false);

    /* iterator */
    class iterator {
     private:
        FeatureVector * _m_fv;
        int _m_ind;
       
     private: 
        iterator(FeatureVector *fv, int ind) : 
            _m_fv(fv), _m_ind(ind) { }
     public:
        iterator() : _m_fv(NULL), _m_ind(0) { }
        iterator(const iterator &orig) : 
            _m_fv(orig._m_fv), _m_ind(orig._m_ind) { }

        ~iterator() { }

        iterator& operator++();
        iterator  operator++(int);
        iterator& operator--();
        iterator  operator--(int);
        bool      operator==(const iterator &) const;
        bool      operator!=(const iterator &) const;
        Feature*  operator*();
    
        friend class FeatureVector;
    };

    const iterator & begin() const { return *_begin; };
    const iterator & end() const { return *_end; }

 private:
    bool hasmore(int index);
    Feature * get(int index);

 private:
    iterator * _begin;
    iterator * _end;
    bool _limited;
    vector<Feature *> _feats;

    // Generators
    Lookup<IdiomFeature> iflookup;
    Lookup<OperandFeature> oflookup;


 friend class FeatureVector::iterator;
};

#endif
