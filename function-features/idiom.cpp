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
#include <cstdio>

#include "CodeObject.h"
#include "Function.h"
#include "InstructionDecoder.h"
#include "Instruction.h"
#include "entryIDs.h"
#include "RegisterIDs.h"

#include "feature.h"


using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;
using namespace NS_x86;


/** IdiomFeature implementation **/

IdiomTerm::IdiomTerm(Function *f, Address addr) :
    entry_id(ILLEGAL_ENTRY),
    arg1(NOARG),
    arg2(NOARG),
    len(0)
{
    Instruction insn;
    InstructionDecoder dec(
        (unsigned char*)f->isrc()->getPtrToInstruction(addr),
        30, // arbitrary
        f->isrc()->getArch());

    insn = dec.decode();
    if(insn.size() > 0 && insn.isValid()) { 
        // set representation

        len = insn.size();

        const Operation & op = insn.getOperation();
        entry_id = op.getID() + 1;

//        fprintf(stderr,"IT[%d,%d",entry_id,len);

        if(entry_id != ILLEGAL_ENTRY) {
            vector<Operand> ops;
            insn.getOperands(ops);
//            fprintf(stderr,",%ld",ops.size());
            
            // we'll take up to two operands... which seems bad. FIXME

            int args[2] = {NOARG,NOARG};
            
            for(unsigned int i=0;i<2 && i<ops.size();++i) {
                Operand & op = ops[i];
                if(!op.readsMemory() && !op.writesMemory()) {
                    // register or immediate
                    set<RegisterAST::Ptr> regs;
                    op.getReadSet(regs);
                    op.getWriteSet(regs);

                    if(!regs.empty()) {
//		        if (regs.size() > 2) fprintf(stderr, "%s touches more than two registers in one operand\n", insn->format().c_str());
		        if (regs.size() > 1)
			    args[i] = MULTIREG;
			else
			    args[i] = (*regs.begin())->getID();
                    } else {
                        // immediate
                        args[i] = IMMARG;
                    }
                } else {
                    args[i] = MEMARG; 
                }
            }

            arg1 = args[0];
            arg2 = args[1];
        } else {
            //fprintf(stderr,",0");
        }


//        fprintf(stderr,",%x,%x],%s\n",arg1,arg2,insn->format().c_str());
    } else {
//        fprintf(stderr,"IT[no insn decoded]\n");
    }
    if (entry_id - 1 == e_nop) {
        arg1 = arg2 = NOARG;
    }
}

IdiomTerm::IdiomTerm(SymtabCodeSource *sts, Address addr) :
    entry_id(ILLEGAL_ENTRY),
    arg1(NOARG),
    arg2(NOARG),
    len(0)
{
    Instruction insn;
    InstructionDecoder dec(
        (unsigned char*)(sts->getPtrToInstruction(addr)),
        30, // arbitrary
        sts->getArch());

    insn = dec.decode();
    if(insn.size() > 0 && insn.isValid()) { 
        // set representation

        len = insn.size();

        const Operation & op = insn.getOperation();
        entry_id = op.getID() + 1;
//        fprintf(stderr,"IT[%d,%d",entry_id,len);

        if(entry_id != ILLEGAL_ENTRY) {
            vector<Operand> ops;
            insn.getOperands(ops);
//            fprintf(stderr,",%ld",ops.size());
            
            // we'll take up to two operands... which seems bad. FIXME

            int args[2] = {NOARG,NOARG};
            
            for(unsigned int i=0;i<2 && i<ops.size();++i) {
                Operand & op = ops[i];
		if (op.getValue()->size() == 0) {
		    // This is actually an invalid instruction with valid opcode
		    len = 0;
		    return;
		}
                if(!op.readsMemory() && !op.writesMemory()) {
                    // register or immediate
                    set<RegisterAST::Ptr> regs;
                    op.getReadSet(regs);
                    op.getWriteSet(regs);

                    if(!regs.empty()) {
		        if (regs.size() > 2) {
			    fprintf(stderr, "%s touches more than two registers in one operand\n", insn.format().c_str());
			    exit(0);
			}
		        if (regs.size() > 1) {
			    MachRegister r1 = (*regs.begin())->getID();
			    MachRegister r2 = (*(--regs.end()))->getID();
			    if (r1 == InvalidReg || r2 == InvalidReg) {
			        fprintf(stderr, "size == 0 heuristic doesn't work very well...\n");
				exit(0);
			    }
			    args[i] = MULTIREG;
			}
			else {
			    args[i] = (*regs.begin())->getID();
			    if (MachRegister(args[i]) == InvalidReg) {
				fprintf(stderr, "invalid reg\n");
				exit(0);
			    }
			    if (args[i] == NOARG || args[i] == IMMARG || args[i] == MEMARG || args[i] == MULTIREG) {
			        fprintf(stderr, "valid MachRegister is used for non-register\n");
				exit(0);
			    } 
			}
                    } else {
                        // immediate
                        args[i] = IMMARG;
                    }
                } else {
                    args[i] = MEMARG; 
                }
            }

            arg1 = args[0];
            arg2 = args[1];
        } else {
            //fprintf(stderr,",0");
        }


//        fprintf(stderr,",%x,%x],%s\n",arg1,arg2,insn->format().c_str());
    } else {
//        fprintf(stderr,"IT[no insn decoded]\n");
    }
}

static string HandleAnOperand(unsigned short arg, int style) {
    if(arg != NOARG) {
	if (arg == MEMARG) {
	    if (style) return "MEM"; else return ":A";
	}
	else if (arg == IMMARG) {
	    if (style) return "IMM"; else return ":B";
	}
	else if (arg == MULTIREG) {
	    if (style) return "MULTIREG"; else return ":C";
	} 
	else if (arg == CALLEESAVEREG) {
	    if (style) return "Callee saved reg"; else return ":D";
	} 
	else if (arg == ARGUREG) {
	    if (style) return "Argu passing reg"; else return ":E";
	}
	else if (arg == OTHERREG) {
	    if (style) return "Other reg"; else return ":F";
	}
	else {
	    // the human_readble format is still broken
	    if (style) {
		if (arg == 0x0010) return x86_64::rip.name();
	        switch (arg & 0xf000) {
		    case 0x0000:
		        //GPR
			return MachRegister(arg | x86_64::GPR | Arch_x86_64).name();
		    case 0x1000:
		        //mmx
			return MachRegister(arg | x86_64::MMX | Arch_x86_64).name();
		    case 0x2000:
		        //xxm
			return MachRegister(arg | x86_64::XMM | Arch_x86_64).name();
		    case 0x4000:
		        // flag bit
			return MachRegister(arg | x86_64::FLAG | Arch_x86_64).name();
		}

	      
	        return ":" + MachRegister(arg).name();
	    }
	    char buf[64];
	    snprintf(buf, 64, "%x", arg);
	    return string(":") + string(buf);
	}
    }
    return "";
}

extern IdiomTerm WILDCARD_IDIOM;
string
IdiomTerm::human_format() 
{
    string entryname;
    if(*this == WILDCARD_IDIOM)
        entryname = "*";
    else if(entryNames_IAPI.find((entryID)(entry_id-1)) == entryNames_IAPI.end()) {
        // Just not lack of entry in entryNames_IAPI, once found such situation, should add an entry
        entryname = "[UNMAPED]";
	fprintf(stderr, "Found entryID not mapped in entryNames_IAPI %d\n", entry_id - 1);
    }
    else {
        entryname = entryNames_IAPI[(entryID)(entry_id-1)];
	if (arg1 != NOARG) entryname += " " + HandleAnOperand(arg1, 1);
	if (arg2 != NOARG) entryname += "," + HandleAnOperand(arg2, 1);
    }

    return entryname;        
}

IdiomTerm::IdiomTerm(uint64_t it) 
{
    from_int(it);
}


string
IdiomTerm::format() {
    string _format;
    if (*this == WILDCARD_IDIOM)
        _format = "*";
    else {
        char buf[64];
        snprintf(buf, 64, "%x", entry_id);
	_format += buf + HandleAnOperand(arg1, 0)  + HandleAnOperand(arg2, 0); 
    }
    return _format;
}

bool
IdiomTerm::operator==(const IdiomTerm &it) const {
    
    return entry_id == it.entry_id &&
           arg1 == it.arg1 &&
           arg2 == it.arg2;
}
bool
IdiomTerm::operator<(const IdiomTerm &it) const {
    if(entry_id < it.entry_id) 
        return true;
    else if(entry_id == it.entry_id) {
        if(arg1 < it.arg1) 
            return true;
        else if(arg1 == it.arg1) 
            if(arg2 < it.arg2)
                return true;
    }
    return false;
}

size_t
IdiomTerm::hash() const {
    return to_int();
}

uint64_t
IdiomTerm::to_int() const {
    uint64_t ret = 0;

    ret += ((uint64_t)entry_id) << 32;
    ret += ((uint64_t)arg1) << 16;
    ret += ((uint64_t)arg2);

    return ret;

}

void
IdiomTerm::from_int(uint64_t it)
{
    entry_id = (it>>32) & 0xffffffff;
    arg1 = (it>>16) & 0xffff;
    arg2 = it & 0xffff;
}

static void split(const char * str, vector<uint64_t> & terms)
{
    const char *s = str, *e = NULL;
    char buf[32];

    while((e = strchr(s,'_')) != NULL) {
        assert(e-s < 32);

        strncpy(buf,s,e-s);
        buf[e-s] = '\0';
        terms.push_back(strtoull(buf,NULL,16));

        s = e+1;
    }
    // last one
    if(strlen(s)) {
        terms.push_back(strtoull(s,NULL,16));
    }
 
}
string 
IdiomFeature::human_format() {
    string ret = "";

    //printf("formatting %s\n",format().c_str());
    for(unsigned i=0;i<_terms.size();++i) {
        ret += ((IdiomTerm*)_terms[i])->human_format();
        if(i<_terms.size()-1)
            ret += "|";
    }
    return ret;
}

IdiomFeature::IdiomFeature(char * str) 
{
    vector<uint64_t> terms;
    //printf("init from **%s**\n",str);
    split(str+1,terms);
    for(unsigned i=0;i<terms.size();++i) {
        // XXX output may have had NOARGs shifted off.
        //     need to shift them back on.

        // we enforce a "no zero entry_id" policy,
        // which makes this safe
        uint64_t t = terms[i];
        if(!(t & ENTRY_MASK)) {
            t = t << ARG_SIZE;
            t |= NOARG;
        }
        if(!(t & ENTRY_MASK)) {
            t = t << ARG_SIZE;
            t |= NOARG;
        }

        //printf("    %lx\n",terms[i]);
        add_term(new IdiomTerm(t));
    }
}

string
IdiomFeature::format() {
    string _format;

    char buf[64];

    _format = "I";

    for(unsigned i=0;i<_terms.size();++i) {
        // XXX shrink output by removing NOARGs from right
//        if (*(IdiomTerm*)_terms[i] == WILDCARD_IDIOM) {
//	    if (i+1<_terms.size()) _format += "*_"; else _format += "*";
//	} else {
	    uint64_t out = ((IdiomTerm*)_terms[i])->to_int();
	    if((out & 0xffff) == NOARG)
	        out = out >> 16;
	    if((out & 0xffff) == NOARG)
	        out = out >> 16;
	    if(i+1<_terms.size())
	        snprintf(buf,64,"%lx_",out);
	    else
	        snprintf(buf,64,"%lx",out);
	    _format += buf;
//	}
    }

    return _format;
}

bool IdiomFeature::operator< (const IdiomFeature &f) const {
    if (_terms.size() != f._terms.size()) return _terms.size() < f._terms.size();
    for (size_t i = 0; i < _terms.size(); ++i)
        if (*(IdiomFeature*)(_terms[i]) < *(IdiomFeature*)(f._terms[i])) return true;
	else if (*(IdiomFeature*)(f._terms[i]) < *(IdiomFeature*)(_terms[i])) return false;
    return false;
}
