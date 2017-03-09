import sys
import os
import math
import argparse
from subprocess import *

compilerList = ["GCC", "ICC", "LLVM"]
optimizationList = ["O0", "O1", "O2", "O3", "Os"]

def getParameters():
    parser = argparse.ArgumentParser(description='Generate training files and testing files for each fold')
    parser.add_argument("--filelist", help="The list of binaries included in the data set", required = True)
    parser.add_argument("--outputdir", help ="The root directory to store data files and summary files", required=True)
    args = parser.parse_args()
    return args

def GetFiles(filelist):
    files = open(filelist, "r").read().split("\n")[:-1]
    return files

def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None

args = getParameters()
files = GetFiles(args.filelist)
binaries = {}
for line in files:
    parts = line.split("/")
    proj, compiler, version, opt = GetToolchainInfo(parts)
    t = (compiler, opt)
    if t not in binaries:
        binaries[t] = {}
    filename = parts[-1]
    binaries[t][filename] = line

toolchainList = []
for c in compilerList:
    for o in optimizationList:
        toolchainList.append((c, o))

fold = 0
for fname1 in binaries[("GCC", "O0")]:
    f1 = open(os.path.join(args.outputdir, "{0}.train".format(fold)), "w")
    f2 = open(os.path.join(args.outputdir, "{0}.test".format(fold)), "w")
    for fname2 in binaries[("GCC", "O0")]:
        if fname1 == fname2:
	    f = f2
	else:
	    f = f1
	for t in toolchainList:
	    f.write(binaries[t][fname2]+ "\n")
    f1.close()
    f2.close()
    fold += 1
print "Total fold =", fold
