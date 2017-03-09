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
#include <assert.h>
#include <stdio.h>

#include "CodeObject.h"
#include "InstructionDecoder.h"
#include "Instruction.h"

#include "feature.h"
#include "iapihax.h"

#define MAX_IDIOM_LEN 3
IdiomTerm WILDCARD_IDIOM(0xaaaaffffffff);

/* op1 x x op2 */
#define MAX_OPERAND_DIST 3
OperandTerm WILDCARD_OPERAND_D1(0x1ffff);
OperandTerm WILDCARD_OPERAND_D2(0x2ffff);
OperandTerm * OP_WC[] = { &WILDCARD_OPERAND_D1, &WILDCARD_OPERAND_D2 };


using namespace std;
using namespace __gnu_cxx;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;

/*
template<typename T>
void Lookup<T>::lookup(Address addr, vector<Feature *> & feats)
{
    fprintf(stderr,"generic lookup invoked\n");
}
*/

template<>
void Lookup<IdiomFeature>::lookup_idiom(
    int size,
    Function * f,
    Address addr,
    Address end,
    Lookup<IdiomFeature>::Lnode * cur,
    int depth,
    vector<LookupTerm *> & stack,
    vector<Feature *> & feats)
{
    Lookup<IdiomFeature>::Lnode * next;
    IdiomTerm * ct;
    
    if(depth >= size || !f->isrc()->getPtrToInstruction(addr))
        return;

    // Make sure idioms don't pass the end of a basic block
    if (addr >= end) return;

    if(_term_map.find(addr) == _term_map.end())
        _term_map[addr].push_back(new IdiomTerm(f,addr));
    ct = *_term_map[addr].begin();

    stack.push_back(ct);

    // non-wildcard
    next = cur->next(ct);
    if(!next->f && !fixed)
        next->f = new IdiomFeature( stack );
    if(next->f && depth+1 == size) {
        feats.push_back(next->f);
    }
    if(ct->entry_id != ILLEGAL_ENTRY)
        lookup_idiom(size, f,addr+ct->len,end, next,depth+1,stack,feats);

    stack.pop_back();

    // wildcard
    stack.push_back(&WILDCARD_IDIOM);
    next = cur->next(&WILDCARD_IDIOM);
    if(!next->f && !fixed)
        next->f = new IdiomFeature( stack );
    if(ct->entry_id != ILLEGAL_ENTRY)
        lookup_idiom(size, f,addr+ct->len,end, next,depth+1,stack,feats);
    
    stack.pop_back();
}
template<>
void Lookup<IdiomFeature>::lookup(
    int size, Function * f, Address addr, Address end, vector<Feature *> & feats)
{
    vector<LookupTerm *> stack;
    lookup_idiom(size, f,addr,end, &start,0,stack,feats);
}

template<>
void Lookup<IdiomFeature>::lookup_idiom_cross_blk(
    int size,
    Function * f,
    Block * b, 
    Address addr,
    Lookup<IdiomFeature>::Lnode * cur,
    int depth,
    vector<LookupTerm *> & stack,
    vector<Feature *> & feats)
{
    Lookup<IdiomFeature>::Lnode * next;
    IdiomTerm * ct;
    

    if(depth >= size || !f->isrc()->getPtrToInstruction(addr))
        return;

    if(_term_map.find(addr) == _term_map.end())
        _term_map[addr].push_back(new IdiomTerm(f,addr));
    ct = *_term_map[addr].begin();

    stack.push_back(ct);

    // non-wildcard
    next = cur->next(ct);
    if(!next->f && !fixed)
        next->f = new IdiomFeature( stack );
    if(next->f && depth+1 == size) {
        feats.push_back(next->f);
    }

    if(ct->entry_id != ILLEGAL_ENTRY) {
        if (addr == b->last()) {
	    // We reach the end of the block, jump to sucessor blocks
	    for (auto eit = b->targets().begin(); eit != b->targets().end(); ++eit) {
	        Edge *e = *eit;
		if (e->sinkEdge() || e->interproc()) continue;
		lookup_idiom_cross_blk(size, f, e->trg(), e->trg()->start(), next, depth+1, stack, feats);
	    }
	        
	} else {
	    // Continue in the same block
	    lookup_idiom_cross_blk(size, f, b, addr+ct->len,next,depth+1,stack,feats);
	}
    }

    stack.pop_back();

    // wildcard
    stack.push_back(&WILDCARD_IDIOM);
    next = cur->next(&WILDCARD_IDIOM);
    if(!next->f && !fixed)
        next->f = new IdiomFeature( stack );

    if(ct->entry_id != ILLEGAL_ENTRY) {
        if (addr == b->last()) {
	    // We reach the end of the block, jump to sucessor blocks
	    for (auto eit = b->targets().begin(); eit != b->targets().end(); ++eit) {
	        Edge *e = *eit;
		if (e->sinkEdge() || e->interproc()) continue;
		lookup_idiom_cross_blk(size, f, e->trg(), e->trg()->start(), next, depth+1, stack, feats);
	    }
	        
	} else {
	    // Continue in the same block
	    lookup_idiom_cross_blk(size, f, b, addr+ct->len,next,depth+1,stack,feats);
	}
    }

    stack.pop_back();
}

template<>
void Lookup<IdiomFeature>::lookup_cross_blk(
    int size, Function * f, Block *b, Address addr, vector<Feature *> & feats)
{
    vector<LookupTerm *> stack;
    lookup_idiom_cross_blk(size, f, b, addr,&start,0,stack,feats);
}

void
get_operands(
    Function *f, Address addr, 
    vector<OperandTerm *> & operands)
{
    Instruction::Ptr insn;

    if(!f->isrc()->getPtrToInstruction(addr))
        return;
    
    InstructionDecoder dec(
        (unsigned char*)f->isrc()->getPtrToInstruction(addr),
        30, //arbitrary
        f->isrc()->getArch());
    insn = dec.decode();
    if(insn) {
        vector<Operand> ops;
        ExpTyper typer;
        insn->getOperands(ops);
        for(unsigned int i=0;i<ops.size();++i) {
            Operand & op = ops[i];
            unsigned short opid;
            bool write = false;

            op.getValue()->apply(&typer);
            if(typer.reg()) {
                set<RegisterAST::Ptr> w_regs, r_regs;
                op.getReadSet(r_regs);
                op.getWriteSet(w_regs);
                write = !w_regs.empty();

                if(!r_regs.empty()) 
                    opid = (*r_regs.begin())->getID();
                else if(!w_regs.empty())
                    opid = (*w_regs.begin())->getID();
                else {
                    assert(0); // XXX Debugging
                    continue;
                }
            } else if(!typer.imm()) { 
                write = op.writesMemory();
                opid = MEMARG;
            } else {
                continue;
            }


            OperandTerm * ot = new OperandTerm(opid,write);
            //fprintf(stderr,"    %s [%lx] %d %d\n",op.format().c_str(),ot->to_int(), op.readsMemory(), op.writesMemory());

            operands.push_back(ot);
        }
    }
}


template<>
void Lookup<OperandFeature>::lookup(
    int size, Function *f, Address addr, Address end, vector<Feature *> & feats)
{
    Instruction::Ptr insn;

    /* for each operand here, we want to record a new feature for bigrams
       with operands up to MAX_OPERAND_DIST away */
    if (addr >= end) return;
    if(_term_map.find(addr) == _term_map.end())
        get_operands(f,addr,_term_map[addr]);

    vector<OperandTerm *> & terms1 = _term_map[addr];
    
    Address cur = addr;
    InstructionDecoder dec(
        (unsigned char *)f->isrc()->getPtrToInstruction(addr),
        f->isrc()->offset() + f->isrc()->length() - addr,
        f->isrc()->getArch());

        unsigned d = size;
        if(!f->isrc()->getPtrToInstruction(cur))
            return;
        insn = dec.decode((unsigned char*)f->isrc()->getPtrToInstruction(addr));
        cur += insn->size(); 

        if(_term_map.find(cur) == _term_map.end())
            get_operands(f,cur,_term_map[cur]);

        vector<OperandTerm *> & terms2 = _term_map[cur];

        // need a distance-d wildcard
        OperandTerm * wc = NULL;
        if(d > 0) {
            wc = OP_WC[d-1];
        } 

        for(unsigned i=0;i<terms1.size();++i) {
            OperandTerm * ot1 = terms1[i];
            Lnode * node = start.next(ot1);
            if(wc)
                node = node->next(wc);
 
            for(unsigned j=0;j<terms2.size();++j) {
                OperandTerm * ot2 = terms2[j];
                node = node->next(ot2); 

                if(!node->f && !fixed) {
                    node->f = new OperandFeature();
                    node->f->add_term(ot1);
                    if(wc)
                        node->f->add_term(wc);
                    node->f->add_term(ot2);
                }
                if(node->f)
                    feats.push_back(node->f);
            }
        }
}

template<typename T>
Lookup<T>::~Lookup() 
{
    typename unordered_map<Address, vector<LT *> >::iterator tit = _term_map.begin();
    for( ; tit != _term_map.end(); ++tit)
        for(unsigned i=0;i<tit->second.size();++i) 
            delete (tit->second)[i];
}

template<typename T>
typename Lookup<T>::Lnode *
Lookup<T>::Lnode::next(LT *t)
{
    if(_next.find(*t) == _next.end()) {
        _next[*t] = new Lnode();
    }
    return _next[*t];
}


template<typename T>
Lookup<T>::Lnode::Lnode() :
    f(NULL)
{

}

template<typename T>
Lookup<T>::Lnode::~Lnode()
{
    if(f)
        delete f;

    typename lmap_t::iterator nit = _next.begin();
    for( ; nit != _next.end(); ++nit) {
        delete (*nit).second;
    }
}





/* Required because of partial specialization of lookup */
template class Lookup<IdiomFeature>;
template class Lookup<OperandFeature>;

