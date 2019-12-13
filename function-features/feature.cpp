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
#include<assert.h>

#include "InstructionDecoder.h"
#include "Instruction.h"

#include "CodeObject.h"
#include "CFG.h"

#include "feature.h"

using namespace Dyninst::InstructionAPI;

/** iterator implementation **/

FeatureVector::iterator&
FeatureVector::iterator::operator++() {
    if(_m_fv->hasmore(_m_ind))
        ++_m_ind;
    else
        _m_ind = -1;
    return *this;
}

FeatureVector::iterator
FeatureVector::iterator::operator++(int) {
    FeatureVector::iterator copy(*this);
    if(_m_fv->hasmore(_m_ind))
        ++_m_ind;
    else
        _m_ind = -1;
    return copy;
}

FeatureVector::iterator&
FeatureVector::iterator::operator--() {
    if(_m_ind > 0)
        --_m_ind;
    return *this;
}

FeatureVector::iterator
FeatureVector::iterator::operator--(int) {
    FeatureVector::iterator copy(*this);
    if(_m_ind > 0)
        --_m_ind;
    return copy;
}

bool
FeatureVector::iterator::operator==(const FeatureVector::iterator & obj) const {
    return obj._m_fv == _m_fv && obj._m_ind == _m_ind;
}

bool
FeatureVector::iterator::operator!=(const FeatureVector::iterator & obj) const {
    return !(*this == obj);
}

Feature*
FeatureVector::iterator::operator*() {
    return _m_fv->get(_m_ind);
}

/** generic lookup feature implementation **/
void
LookupFeature::add_term(LookupTerm *t)
{
    _terms.push_back(t);
}

LookupFeature::LookupFeature(vector<LookupTerm *> & t)
{
    _terms.insert(_terms.begin(),t.begin(),t.end());
}

/** feature vector implementation **/

FeatureVector::FeatureVector() :
    _begin(new iterator(this,-1)),
    _end(new iterator(this,-1)),
    _limited(false)
{

}

FeatureVector::FeatureVector(char * featfile) :
    _begin(new iterator(this,-1)),
    _end(new iterator(this,-1)),
    _limited(true)
{
    // FIXME load limited feature descriptions
    assert(0);
}


FeatureVector::~FeatureVector() {
    if(_begin)
        delete _begin;
    if(_end)
        delete _end;
}

int
FeatureVector::eval(int size, Function *f, bool idioms, bool operands) {

    _feats.clear();
    (*_begin) = (*_end);

    Function::blocklist::iterator bit = f->blocks().begin();
    for( ; bit != f->blocks().end(); ++bit) {
        Block * b = *bit;
        Block::Insns insns;
        b->getInsns(insns);
        for (auto iit = insns.begin(); iit != insns.end(); ++iit) {
            if(idioms)
                iflookup.lookup(size, f, iit->first, b->end(), _feats);
            if(operands)
                oflookup.lookup(size, f, iit->first, b->end(), _feats);
        }
    }
    if(!_feats.empty())
        _begin->_m_ind = 0;
        

    return _feats.size();
}

int
FeatureVector::eval_cross_blk(int size, Function *f, bool idioms, bool operands) {
    _feats.clear();
    (*_begin) = (*_end);

    Function::blocklist::iterator bit = f->blocks().begin();
    for( ; bit != f->blocks().end(); ++bit) {
        Block * b = *bit;
        Block::Insns insns;
        b->getInsns(insns);
        for (auto iit = insns.begin(); iit != insns.end(); ++iit) {
            if(idioms)
                iflookup.lookup_cross_blk(size, f, b, iit->first,_feats);
            if(operands)
                oflookup.lookup(size, f, iit->first, b->end(), _feats);
        }
    }

    if(!_feats.empty())
        _begin->_m_ind = 0;
        

    return _feats.size();
}

bool
FeatureVector::hasmore(int index) {
    return index < ((int)_feats.size())-1;
}

Feature *
FeatureVector::get(int index) {
    if(index < (int)_feats.size())
        return _feats[index];
    else
        return NULL;
}
