# Toolchain-origin
This tool uses machine learning to identify the compilation toolchains 
that generated a binary program. The compilation toolchain refers to 
the compiler family, compiler version, and compiler flags that were 
used generated a binary. Toolchain identification is performed at the 
function level, meaning that each function is attributed to a single toolchain.

# Initial support
The current version supports identifying three compiler families 
(GCC, ICC and LLVM) and five optimization flags (-O0, -O1, -O2, 
-O3, and -Os).

# Install

The short version:
1. First edit file ``function-features/Makefile'' and change 
variable DYNINST_DIR to your dyninst install location
2. ./install.sh <path-to-install>

In more details, this toolchain contains three components: 
lbfgs library, crfsuite, and feature extraction.

# Usage
To be written

# Train your own model
To be written


