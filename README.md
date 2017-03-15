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


