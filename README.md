# toolchain-origin
Identifying the compiler family, version and compiler flags that generated a binary

Initial support:

Three compiler family: GCC, ICC, LLVM

Five optimization flags: -O0, -O1, -O2, -O3, -Os


Steps for getting to the first release:
1. Set up CMake environment to find externally installed Dyninst
2. Train on a large set of binaries to get a model
2.1 Need to make sure that features representation can be reused across different Dyninst versions
2.2 Put the model in the repo
3. Prepare a simple user interface
