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

#include "CodeObject.h"
#include "Function.h"
#include "InstructionDecoder.h"
#include "Instruction.h"

#include "feature.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;

OperandTerm::OperandTerm(uint64_t it) :
    _formatted(false)
{
    from_int(it);
}

string
OperandTerm::format() {
    if(_formatted)
        return _format;

    char buf[64];
    if(_wc_dist > 0) 
        snprintf(buf,64,"*%d",_wc_dist);
    else 
        snprintf(buf,64,"%s%x",
            _read  ? "R" : "W",
            _id);

    _format += buf;
    _formatted = true;
    return _format;
}

bool
OperandTerm::operator==(const OperandTerm &it) const {
    return _id == it._id && _wc_dist == it._wc_dist &&
        _write == it._write;
}
bool
OperandTerm::operator<(const OperandTerm &it) const {
    if(_id < it._id)
        return true;
    else if(_id == it._id) {
        if(_wc_dist < it._wc_dist)
            return true;
        else if(_wc_dist == it._wc_dist)
            if(_write < it._write)
                return true;
    }
    return false;
}

size_t
OperandTerm::hash() const {
//    __gnu_cxx::hash<uint64_t> H; 
    return reinterpret_cast<size_t>((void*)to_int());
}

uint64_t
OperandTerm::to_int() const {
    uint64_t ret = 0;

    ret += ((uint64_t)_write) << 24;
    ret += ((uint64_t)_wc_dist) << 16;
    ret += ((uint64_t)_id);

    return ret;

}

void
OperandTerm::from_int(uint64_t it)
{
    _write = (it >> 24) & 0x1;
    _wc_dist = (it >> 16) & 0xff;
    _id = it & 0xffff;
}

string
OperandFeature::format() {
    if(_formatted)
        return _format;

    char buf[64];

    _format = "O";

    for(unsigned i=0;i<_terms.size();++i) {
        if(i+1<_terms.size())
            snprintf(buf,64,"%lx_",((OperandTerm*)_terms[i])->to_int());
        else
            snprintf(buf,64,"%lx",((OperandTerm*)_terms[i])->to_int());
        _format += buf;
    }

    _formatted = true;
    return _format;
}
