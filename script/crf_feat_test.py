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


def GetFiles(filename):
    fList = []
    for fileline in open(filename,"r"):
        parts = fileline[:-1].split("/")
	filename = parts[-1]
	proj, compiler, version, opt = GetToolchainInfo(parts)
	prefix = os.path.join("data" , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
	fList.append(prefix + ".data")
    return fList


def PrintResults(results):
    for l in results:
        for i in l:
	    print i,
	print

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
#	    chainLen += 1
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
	f.write("\n")
    f.close()
    return dataFileName

def CRFTrain(trainFile):
    cmd = "{0} learn -m tmp.crf.model -a lbfgs -p max_iterations={1} {2}".format("./crfsuite", 1500, trainFile)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message in running", cmd,":"
	print err
    print msg

def CRFPredict(testFile):
    cmd = "{0} tag -t -m tmp.crf.model {1}".format("./crfsuite", testFile)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message in running", cmd ,":"
	print err
    print "RESULTS"
    print msg

      

def KeepFeatures(keep, test):
    # build index for kept features
    # feature canoical representation --> integer index
    keepFeat = {}
    scale = {}
    i = 0
    for line in open("fs{0}.txt".format(test)):
        feat = int(line[:-1])
	i += 1
	keepFeat[feat] = i
	if i == keep: break
    allFiles = trainDataFiles + testDataFiles
    for filename in allFiles:
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


testfolds = int(sys.argv[1])
test = int(sys.argv[2])
keep = int(sys.argv[3])
jobNumber = int(sys.argv[4])
trainDataFiles = GetFiles("data/{0}.train".format(test))
testDataFiles = GetFiles("data/{0}.test".format(test))
keptFeatIndex, featScale = KeepFeatures(keep, test)

trainFile = GenerateCRFData(keptFeatIndex, featScale, trainDataFiles, "tmp.crf.train")
testFile = GenerateCRFData(keptFeatIndex, featScale, testDataFiles, "tmp.crf.test")

CRFTrain(trainFile)
CRFPredict(testFile)
