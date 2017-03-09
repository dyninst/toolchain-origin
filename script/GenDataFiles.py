# See README.txt for the details of the experiments

import sys
import os
import math
import argparse
from subprocess import *

featIndex = {}
featList = []
totalFeat = 0
compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def getParameters():
    parser = argparse.ArgumentParser(description='Generate submission files of HTCondor to run experiments to evaluate the accuracy of toolchain identification techniques')
    parser.add_argument("--filelist", help="The list of binaries included in the data set", required = True)
    parser.add_argument("--idiom", help="Use instruction idioms with specified sizes")
    parser.add_argument("--graphlet", help="Use graphelt features with specified sizes")
    parser.add_argument("--outputdir", help ="The root directory to store data files and summary files", required=True)
    parser.add_argument("--featdir", required=True)
    args = parser.parse_args()
    return args

def GetFiles(filelist):
    files = open(filelist, "r").read().split("\n")[:-1]
    return files

def ParseFeatureSize(param):
    if param == None:
        return None
    ret = []
    for size in param.split(':'):
        ret.append(int(size))
    return ret

def GetGlobalFeatureIndex(feat):
    global featIndex
    global featList
    global totalFeat

    if feat not in featIndex:
        totalFeat += 1
	featIndex[feat] = totalFeat
	featList.append(feat)
    return featIndex[feat]



def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None

def BuildToolchainIndexMap(files):
    toolchainIndexMap = {}
    toolchainIndex = 0
    for line in files:
        parts = line.split("/")
        proj, compiler, version, opt = GetToolchainInfo(parts)
	filename = parts[-1]
	toolchain = "{0}-{1}-{2}".format(compiler,version, opt)
	if toolchain not in toolchainIndexMap:
	    toolchainIndex += 1
	    toolchainIndexMap[toolchain] = toolchainIndex

    # Save the author index map to a file
    f = open(os.path.join(args.outputdir, "toolchain-index.txt"), "w")
    toolchainList = []
    for name , index in toolchainIndexMap.iteritems():
        toolchainList.append( (index, name) )
    toolchainList.sort()
    for index, name in toolchainList:
        f.write("{0} {1}\n".format(index, name))
    f.close()

    return toolchainIndexMap

	    
def CountFeatures(featDir, filename, prefixedFileName, featType, featSizes):
    if featSizes == None:
        return
    if featType == "libcall":
        featSizes = [1]
    
    # Iterate over each specified feature size
    for size in featSizes:       
	featFileNamePrefix = "{0}.{1}.{2}".format(os.path.join(featDir,filename), featType, size)

	# Build feature index for this particular feature type and size
	# local index --> canoical feature representation
	try:
	    localFeatIndex = {}
	    for line in open(featFileNamePrefix + ".featlist", "r"):
	        index, feat = line[:-1].split()
		localFeatIndex[index] = feat

	    for line in open(featFileNamePrefix + ".instances", "r"):
	        parts = line[:-1].split()
		addr = parts[0]
		for featPair in parts[1:]:
		    feat, count = featPair.split(":")
		    if float(count) == 0: continue
		    featIndex = GetGlobalFeatureIndex(localFeatIndex[feat])	  
	except IOError as e:
	    print e


def BuildGlobalFeatureIndex(files):

    # iterate over all features files to index all features
    for line in files:
        parts = line.split("/")
        proj, compiler, version, opt = GetToolchainInfo(parts)
        filename = parts[-1]
	featDir = os.path.join(args.featdir, proj, compiler, version, opt)
	prefixedFileName = "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename)
	CountFeatures(featDir, filename, prefixedFileName, "idiom", idiomSizes)
	CountFeatures(featDir, filename, prefixedFileName, "graphlet",  graphletSizes)
    f = open(os.path.join(args.outputdir, "feat_list.txt"), "w")
    for i in range(totalFeat):
        f.write("{0} {1}\n".format(i + 1, featList[i]))
    f.close()

def KeepFeatures(keep, MIRank):
    # build index for kept features
    # feature canoical representation --> integer index
    keepFeat = {}
    for i in range(keep):
        keepFeat[featList[MIRank[i][1] - 1]] = i+1
    return keepFeat

def AddFeatures(featDir, filename, prefixedFileName, featType, featSizes, dataInstances):
    if featSizes == None:
        return
    if featType == "libcall":
        featSizes = [1]
    
    # Iterate over each specified feature size
    for size in featSizes:       
	featFileNamePrefix = "{0}.{1}.{2}".format(os.path.join(featDir,filename), featType, size)

	# Build feature index for this particular feature type and size
	# local index --> canoical feature representation
	localFeatIndex = {}
	for line in open(featFileNamePrefix + ".featlist", "r"):
	    index, feat = line[:-1].split()
	    localFeatIndex[index] = feat

	for line in open(featFileNamePrefix + ".instances", "r"):
	    parts = line[:-1].split()
	    addr = parts[0]
	    if addr not in dataInstances: dataInstances[addr] = []
	    for featPair in parts[1:]:
	        feat, count = featPair.split(":")
		count = float(count)
		if count == 0: continue
		if localFeatIndex[feat] not in featIndex: continue
		featI = featIndex[localFeatIndex[feat]]
		dataInstances[addr].append( (featI, count) ) 

def GetAllAddresses(featDir, filename, featType, featSize):
    addrList = []
    funcFileName = "{0}.{1}.{2}.instances".format(os.path.join(featDir,filename), featType, featSize)
    for line in open(funcFileName, "r"):
        addr = line[:-1].split()[0]
	addrList.append(addr)
    return addrList


def GenerateCRFData(line):
    parts = line.split("/")
    proj, compiler, version, opt = GetToolchainInfo(parts)
    filename = parts[-1]

    prefix = os.path.join(args.outputdir , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
    prefixedFileName = "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename)
    toolchain = "{0}-{1}-{2}".format(compiler,version, opt)
    label = toolchainIndexMap[toolchain] 
    f = open(prefix + ".data", "w")
    mapping = open(prefix + ".map", "w")
    featDir = os.path.join(args.featdir, proj, compiler, version, opt)
    # addr --> [(kept feat index, count)]
    dataInstances = {}

    AddFeatures(featDir, filename, prefixedFileName, "idiom", idiomSizes, dataInstances)
    AddFeatures(featDir, filename, prefixedFileName, "graphlet", graphletSizes, dataInstances)

    # For CRF data files, we need to group adjacent functions together
    # to form a linear chain
    mapping.write(filename + "\n")
    addrLists = GetAllAddresses(featDir, filename, "idiom", 1)
    inChain = False
    for addr in addrLists:
        if addr not in dataInstances:
	    if inChain:
	        f.write("\n")
	    inChain = False
	else:
	    inChain = True
	    newLine = "{0}".format(label)	
	    dataInstances[addr].sort()
	    for i in range(len(dataInstances[addr])):
	        if i == 0 or dataInstances[addr][i][0] != dataInstances[addr][i-1][0]:
		    newLine += "\t{0}:{1}".format(dataInstances[addr][i][0], dataInstances[addr][i][1])
	    newLine += "\n"
	    f.write(newLine)
	    mapping.write("{0}\n".format(addr))
    f.close()
    mapping.close()

args = getParameters()
files = GetFiles(args.filelist)
idiomSizes = ParseFeatureSize(args.idiom)
graphletSizes = ParseFeatureSize(args.graphlet)

toolchainIndexMap = BuildToolchainIndexMap(files)

# Build a global feature index
print "Start to build global feature index ..."
BuildGlobalFeatureIndex(files)
print "There are {0} features in total".format(totalFeat)

# Generate all data files

print "Start to generate data"
for line in files:
    GenerateCRFData(line)
print "All done"
