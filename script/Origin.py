import sys
import argparse
import os
from subprocess import *

compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def getParameters():
    parser = argparse.ArgumentParser(description='Identify the compiler and optimiation level that generated the binary')
    parser.add_argument("--binpath", help="The path to the binary", required=True)
    parser.add_argument("--workingdir", help="The directory storing extracted features.The default is the current directory", default=".")
    parser.add_argument("--modeldir", help="The directory storing the CRF model and used features", default="data/")

    args = parser.parse_args()
    return args 

def ParseFeatureSize(param):
    if param == None:
        return None
    ret = []
    for size in param.split(":"):
        ret.append(int(size))
    return ret

def GetEndAddress(path, filename):
    out = os.path.join(args.workingdir, "{0}.range".format(filename))
    if not os.path.isfile(out):
        cmd = "{0} {1} {2} {3} {4}".format(featBin, path, "range", 1, out)
	p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
	msg, err = p.communicate()
	if (len(err) > 0):
	    print "Error message:", err
    endAddr = open(out, "r").read()[:-1]
    return endAddr


def Execute(featType, featSize, path, filename):
    out = os.path.join(args.workingdir, "{0}.{1}.{2}".format(filename, featType, featSize))
    if os.path.isfile(out + ".featlist"): return
    cmd = "{0} {1} {2} {3} {4}".format(featBin, path, featType, featSize, out)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message:", err
 

def GenerateFeatures(featType, featSizes, path, filename):
    for size in featSizes:
        Execute(featType, size, path, filename)


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
		if localFeatIndex[feat] not in featureIndex: continue
		featI = featureIndex[localFeatIndex[feat]]
		dataInstances[addr].append( (featI, count / featureScale[featI - 1]) ) 

def GetAllAddresses(featDir, filename, featType, featSize):
    addrList = []
    funcFileName = "{0}.{1}.{2}.instances".format(os.path.join(featDir,filename), featType, featSize)
    for line in open(funcFileName, "r"):
        addr = line[:-1].split()[0]
	addrList.append(addr)
    return addrList


def GenerateCRFData(line):
    parts = line.split("/")
    filename = parts[-1]
    prefix = os.path.join(args.workingdir , "{0}".format(filename))
    f = open(prefix + ".data", "w")
    mapping = open(prefix + ".map", "w")
    # addr --> [(kept feat index, count)]
    dataInstances = {}

    AddFeatures(args.workingdir, filename, prefix, "idiom", idiomSizes, dataInstances)
    AddFeatures(args.workingdir, filename, prefix, "graphlet", graphletSizes, dataInstances)

    # For CRF data files, we need to group adjacent functions together
    # to form a linear chain
    mapping.write(filename + "\n")
    addrLists = GetAllAddresses(args.workingdir, filename, "idiom", 1)
    inChain = False
    for addr in addrLists:
        if addr not in dataInstances:
	    if inChain:
	        f.write("\n")
	    inChain = False
	else:
	    inChain = True
	    newLine = "0"	
	    dataInstances[addr].sort()
	    for i in range(len(dataInstances[addr])):
	        if i == 0 or dataInstances[addr][i][0] != dataInstances[addr][i-1][0]:
		    newLine += "\t{0}:{1:.3f}".format(dataInstances[addr][i][0], dataInstances[addr][i][1])
	    newLine += "\n"
	    f.write(newLine)
	    mapping.write("{0}\n".format(addr))
    f.close()
    mapping.close()
    return prefix + ".data", addrLists

def LoadFeatureData():
    featureFile = os.path.join(args.modeldir, "features.txt")
    index = {}
    scale = []
    count = 0
    for line in open(featureFile, "r"):
        parts = line[:-1].split()
	count += 1
	index[parts[0]] = count
	scale.append(float(parts[1]))
    return index, scale

def LoadLabelString():
    labelIndexFile = os.path.join(args.modeldir, "toolchain-index.txt")
    labelString = {}
    for line in open(labelIndexFile, "r"):
        parts = line[:-1].split()
	label = int(parts[0])
	labelString[label] = parts[1]
    return labelString

def ParseResults(msg):
    labels = []
    for line in msg.split("Performance by label")[0].split("\n"):
        if len(line) < 1: continue
        labels.append(int(line))
    return labels
        

def Predict(testDataFileName, addrList):
    modelFile = os.path.join(args.modeldir, "model.dat")
    cmd = "{0} tag -t -m {1} {2}".format(crfBin, modelFile, testDataFileName)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message in running", cmd ,":"
	print err
    labels = ParseResults(msg)
    return labels

def PrintPrediction(labelString, predict, addrList):
    segments = []
    start = addrList[0]
    l = len(predict)
    count = 1 
    appear = set()
    for i in range(l - 1):
        if predict[i] != predict[i + 1]:
	    segments.append( (predict[i], start, addrList[i+1], count) )
	    start = addrList[i + 1]
	count += 1
	appear.add(predict[i])
    appear.add(predict[l - 1])
    segments.append( (predict[l - 1], start, endAddr, count) )
    col = 0
    column = {}
    print "Address",
    for label in range(1, len(labelString) + 1):
        if label in appear:
	    column[label] = col
	    col += 1
	    print ",", labelString[label],
    print
    for label, start, end, count in segments:
        col = column[label]
        print "[0x{0} - 0x{1})".format(start, end),
	for j in range(len(column)):
	    if j == col:
	        print ",",count,
	    else:
	        print ", 0",
	print
    

args = getParameters()
installdir = "/".join(sys.argv[0].split("/")[:-1])
featBin = os.path.join(installdir, "extractFeat")
crfBin = os.path.join(installdir, "crfsuite")

filename = args.binpath.split("/")[-1]

# We first extract all possible features from the target binary
idiomSizes = [1,2,3]
graphletSizes = [1,2,3]
GenerateFeatures("idiom", idiomSizes, args.binpath, filename)
GenerateFeatures("graphlet", graphletSizes, args.binpath, filename)
endAddr = GetEndAddress(args.binpath, filename)

# We then load the pre-selected features and 
# generate the testing data by only keeping the selected features
# and scaling the feature values
featureIndex, featureScale = LoadFeatureData()
testDataFileName, addrList = GenerateCRFData(args.binpath)

# We invoke crfsuite to perform the prediction
predict = Predict(testDataFileName, addrList)
labelString = LoadLabelString()

PrintPrediction(labelString, predict, addrList)


