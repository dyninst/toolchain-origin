#ifndef _COLORS_H_
#define _COLORS_H_

#include <assert.h>
#include <Instruction.h>

#include <string>

namespace graphlets {
class Color {
 public:
    Color() {}
    virtual ~Color() {}

    virtual unsigned short toint() const { return 0; }
    virtual std::string tostr() const { return ""; }
    virtual std::string compact() const { return "";}

    virtual void merge(Color * /* o */) {}
};

class BranchColor : public Color {
 public:
    enum branch_color {
        JB,
        JB_JNAEJ,
        JBE,
        JCXZ,
        JL,
        JLE,
        JMP,
        JNB,
        JMPE,
        JNB_JAE,
        JNBE,
        JNL,
        JNLE,
        JNO,
        JNP,
        JNS,
        JNZ,
        JO,
        JP,
        JS,
        JZ,
        LOOP,
        LOOPE,
        LOOPN,
        /* 0-22, fitting happily in a byte */

        NOBRANCH
    };

    static branch_color lookup(Dyninst::InstructionAPI::Instruction insn);

    BranchColor(unsigned short s)
      : s_(s) 
    { }

    virtual unsigned short toint() const { return s_; }
    virtual std::string tostr() const { return std::string("BC"); }
    virtual std::string compact() const;

 private:
    unsigned short s_;
};

class InsnColor : public Color {
 public:
    enum insn_color {
        ARITH,
        BRANCH,
        CALL,
        CMP,
        FLAGS,
        FLOATING,
        HALT,
        JUMP,
        LEA,
        LOGIC,
        MOV,
        STACK,
        STRING,
        SYS,
        TEST,
        // 15 bits

        NOCOLOR
    };

    static insn_color lookup(Dyninst::InstructionAPI::Instruction insn);
    static std::map<unsigned short, InsnColor*> insnColorMap;
    static InsnColor* ColorLookup(unsigned short);

    InsnColor(unsigned short s)
      : s_(s) 
    { }

    virtual unsigned short toint() const { return s_; }
    virtual std::string tostr() const { return std::string("IC"); }

    virtual void merge(Color * o) {
        s_ |= ((InsnColor*)o)->s_;
    }
    virtual std::string compact() const;

 private:
    unsigned short s_;
};

#define LOCAL_CALL_NUM ((1<<16)-2)
#define UNKNOWN_LIB_NUM ((1<<16)-1)

class LibCallColor : public Color {
 public:
    LibCallColor(std::string & name)
      :  name_(name)
    { }
    ~LibCallColor() {} 

    virtual unsigned short toint() const {
        assert(!"LibCallColor should not be used as an int");
    }

    virtual std::string tostr() const {
        return name_;
    }

    virtual void merge(Color * O) {
        assert(0);
    }

    virtual std::string compact() const {
        return name_;
    }
    static std::map<std::string, LibCallColor*> libCallColorMap;
    static LibCallColor* ColorLookup(std::string&);
 private:
    std::string name_;
};

class LocalCallColor : public Color {
 public:
    LocalCallColor() { }
    ~LocalCallColor() {}
    
    virtual unsigned short toint() const {
        return LOCAL_CALL_NUM;
    }

    virtual std::string tostr() const {
        return "LOCAL";
    }

    virtual void merge(Color * O) {
        assert(0);
    }
    virtual std::string compact() const {
        return "LOCAL";
    }

    static LocalCallColor localCallColor;
    static LocalCallColor* ColorLookup();
};
}
#endif
