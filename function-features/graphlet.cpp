/*
 * Generates a set of 3-graphlets describing the user-generated portion of
 * the given program binary. 
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <string>
#include <vector>

#include "InstructionDecoder.h"
#include "Instruction.h"

#include "CodeSource.h"
#include "CodeObject.h"
#include "Function.h"
#include "dyntypes.h"

#include "graphlet.h"
#include "colors.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;
using namespace graphlets;

bool COLOR = true;

unsigned short node_insns_color(Block * A)
{
    unsigned short ret = 0;
    Block::Insns insns;
    A->getInsns(insns);
    for (auto iit = insns.begin(); iit != insns.end(); ++iit) {
        InsnColor::insn_color c = InsnColor::lookup(iit->second);    
        if(c != InsnColor::NOCOLOR) {
            assert(c <= 16);
            ret |= (1 << c); 
        }
    }
    return ret; 
}


unsigned short node_branch_color(Block * A)
{
    using namespace Dyninst::InstructionAPI;
    unsigned short ret = BranchColor::NOBRANCH;
    
    Block::Insns insns;
    A->getInsns(insns);
    BranchColor::branch_color c = BranchColor::lookup(insns.rbegin()->second);    
    ret = c; 
    return ret; 
}



bool node::operator<(node const& o) const {
    if(this == &o)
            return false;

	if (color_ != NULL && o.color_ != NULL) {
	    if (color_->compact() != o.color_->compact()) {
	        return color_->compact() < o.color_->compact();
	    }
	} else if (color_ != NULL && o.color_ == NULL) {
	    return false;
	} else if (color_ == NULL && o.color_ != NULL) {
	    return true;
	}
	
	// Continue to compare other fields
	// when 1. both nodes don't have a color;
	// or   2. both nodes have the same color.
        if(ins_ < o.ins_)
            return true;
        else if(o.ins_ < ins_)
            return false;
        else if(outs_ < o.outs_)
            return true;
        else if(o.outs_ < outs_)
            return false;
        else if(self_ < o.self_)
            return true;
        else
            return false;
    }

void node::print() const {
    printf("{"); ins_.print(); printf(" }");
    printf(" {"); outs_.print(); printf(" }");
    printf(" {"); self_.print(); printf(" }");
}

std::string node::toString() const {
    std::string ret;
    ret += "[ {" + ins_.toString() + " }";
    ret += " {" + outs_.toString() + " }";
    ret += " {" + self_.toString() + " } ]";
    return ret;
}

std::string node::compact(bool colors) const {
    std::string ret;
    ret += ins_.compact() + "/" ;
    ret += outs_.compact() + "/";
    ret += self_.compact();
    if(colors && color_ != NULL)
        ret += "/" + color_->compact();
    return ret;
}

