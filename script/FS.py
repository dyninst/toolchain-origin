import sys
import os
import math
import argparse

compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def getParameters():
    parser = argparse.ArgumentParser(description='Perform feature selection')
    parser.add_argument("--filelist", help="A list of binaries for training", required=True)
    parser.add_argument("--datadir", help="The directory storing extracted features", required = True)
    parser.add_argument("--keep", type=int, help="The number of selected features", required= True)
    parser.add_argument("--output", help="The output file of chosen features", required=True)
    args = parser.parse_args()
    return args 


def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None

def GetDataFiles():
    files = []
    for fileline in open(args.filelist, "r"):
        parts = fileline[:-1].split("/")
	filename = parts[-1]
	proj, compiler, version, opt = GetToolchainInfo(parts)
	prefix = os.path.join(args.datadir , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
	files.append( prefix + ".data" )
    return files



def term(xy, x, y):
    if xy < 0.0000001 and xy > -0.0000001 : return 0
    return xy * math.log(xy / x / y)

def Count(pfeat, plabel, plabelfeat):
    total = 0
    for filename in dataFiles:
	for line in open(filename, "r"):
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
    choose = []
    for i in range(args.keep):
        choose.append(int(rank[i][1]))
    return choose

def LoadAllFeatures(featFile):
    f = []
    for line in open(featFile, "r"):
        f.append(line[:-1].split()[1])
    return f

def ScaleFeatures():
    scale = {}
    keepFeat = set()
    for index in choose:
        keepFeat.add(index)
    for filename in dataFiles:
	for line in open(filename, "r"):
	    if len(line) < 2: continue
	    parts = line[:-1].split("\t")
	    for featPair in parts[1:]:
	        feat, count = featPair.split(":")
		count = float(count)
		feat = int(feat)
		if count == 0: continue
		if feat not in keepFeat: continue
		if feat not in scale or count > scale[feat]: scale[feat] = count
    return scale

args = getParameters()
dataFiles = GetDataFiles()
featList = LoadAllFeatures(os.path.join(args.datadir,"feat_list.txt"))
choose = MI()  
featScale = ScaleFeatures()

f = open(args.output, "w")
for index in choose:
    f.write("{0} {1:.3f}\n".format(featList[index - 1], featScale[index]))
f.close()
