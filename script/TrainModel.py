import sys
import os
import math
import argparse
from subprocess import *

compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def getParameters():
    parser = argparse.ArgumentParser(description='Perform feature selection')
    parser.add_argument("--filelist", help="A list of binaries for training", required=True)
    parser.add_argument("--datadir", help="The directory storing the generated data files", required = True)
    parser.add_argument("--featurelist", help="The list of chosen features", required= True)
    parser.add_argument("--path_to_crfsuite", help="The full path to the crfsuite executable", required=True)
    args = parser.parse_args()
    return args 




def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None


def GetFiles(filename):
    fList = []
    for fileline in open(filename,"r"):
        parts = fileline[:-1].split("/")
	filename = parts[-1]
	proj, compiler, version, opt = GetToolchainInfo(parts)
	prefix = os.path.join(args.datadir , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
	fList.append(prefix + ".data")
    return fList

def GenerateCRFData(featIndex, featScale, files, dataFileName):
    f = open(dataFileName,"w")
    chainLen = 0
    for filename in files:
        for line in open(filename, "r"):
	    if len(line) < 2:
	        if chainLen > 0:
		    f.write("\n")
		chainLen = 0
		continue
	    parts = line[:-1].split("\t")
	    newLine = parts[0]
	    featList = []
	    label = int(parts[0])
	    for pair in parts[1:]:
	        feat , count = pair.split(":")
		feat = int(feat)
		count = float(count)
		if count < 1e-5 and count > -1e-5: continue
		if feat not in featIndex: continue
		featList.append ( (featIndex[feat], float(count) / featScale[feat]) )
	    featList.sort()
	    for feat, count in featList:
		newLine += "\t{0}:{1:.3f}".format(feat, count)
	    newLine += "\n"
	    f.write(newLine)
	# We have a sequence of functions for each binary.
	# So we need to insert a new line here to represent it
	f.write("\n")
    f.close()
    return dataFileName

def CRFTrain(trainFile):
    cmd = "{0} learn -m model.dat -a lbfgs -p max_iterations={1} {2}".format(args.path_to_crfsuite, 1500, trainFile)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message in running", cmd,":"
	print err
    print msg

def KeepFeatures():
    # build index for kept features
    # feature canoical representation --> integer index
    keepFeat = {}
    scale = {}
    i = 0
    for line in open(args.featurelist, "r"):    
        i += 1
        index, featStr, feat = line[:-1].split()
	index = int(index)
	keepFeat[index] = i
    for filename in trainDataFiles:
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
    return keepFeat, scale

args = getParameters()
trainDataFiles = GetFiles(args.filelist)
keptFeatIndex, featScale = KeepFeatures()
trainFile = GenerateCRFData(keptFeatIndex, featScale, trainDataFiles, "./tmp.crf.train.txt")
CRFTrain(trainFile)
