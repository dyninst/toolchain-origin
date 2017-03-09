import sys
import os
import math
from subprocess import *

compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None



def term(xy, x, y):
    if xy < 0.0000001 and xy > -0.0000001 : return 0
    return xy * math.log(xy / x / y)

#def PreScanData():

def Count(pfeat, plabel, plabelfeat):
    total = 0
    for fileline in open("data/{0}.train".format(test), "r"):
        parts = fileline[:-1].split("/")
	filename = parts[-1]
	proj, compiler, version, opt = GetToolchainInfo(parts)
	prefix = os.path.join("data" , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
	for line in open(prefix + ".data", "r"):
	    if len(line) < 2: continue
            parts = line[:-1].split("\t")
	    label = parts[0]
	    if label not in plabel:
	        plabel[label] = 0
	    plabel[label] += 1
	    total += 1
	    for featPair in parts[1:]:
	        index, count = featPair.split(":")
		if index not in pfeat:
		    pfeat[index] = 0
		pfeat[index] += 1
		if label not in plabelfeat:
		    plabelfeat[label] = {}
		if index not in plabelfeat[label]:
		    plabelfeat[label][index] = 0
		plabelfeat[label][index] += 1
    return total

def MI():
    pfeat = {}
    plabel = {}
    plabelfeat = {}

    totalInstance = Count(pfeat, plabel, plabelfeat)

    # Calculate probability of the appearance of each author label   
    for label in plabel:
        plabel[label] = float(plabel[label]) / totalInstance

    # Calculate probability of the appearance of each feature   
    for feat in pfeat:
        pfeat[feat] = float(pfeat[feat]) / totalInstance
	
    # Calculate probability of the appearance of each feature and author label pair 
    for label in plabel:
        for feat in pfeat:
	    if feat in plabelfeat[label]:
	        plabelfeat[label][feat] = float(plabelfeat[label][feat]) / totalInstance
    
    # Calculate mutual information for each feature
    rank = []
    for feat in pfeat:
        mi = 0
	for label in plabel:
	    if feat in plabelfeat[label]:    
	        mi += term(plabelfeat[label][feat], plabel[label], pfeat[feat])
	        mi += term(plabel[label] - plabelfeat[label][feat], plabel[label], 1 - pfeat[feat])
	    else:
	        mi += term(plabel[label], plabel[label], 1 - pfeat[feat])
	rank.append( (mi, feat) )
	
    # Sort the features in non-increasing order of their MI
    rank.sort(reverse=True)
    for i in range(keep):
        print rank[i][1]


testFolds = int(sys.argv[1])
test = int(sys.argv[2])
keep = int(sys.argv[3])
#totalInstance, totalLabel = PreScanData(testFolds)
MI()  
