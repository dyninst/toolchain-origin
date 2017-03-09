import sys
import os
import argparse

compilerList = ["GCC", "ICC", "LLVM", "PGI"]
optimization = ["O0", "O1", "O2", "O3", "Os"]
def getParameters():
    parser = argparse.ArgumentParser(description='Collect leave-one-out results for identifying multiple authors (no authorship layout)')
    parser.add_argument("--predictdir", help="The directory storing all prediction results", required = True)
    parser.add_argument("--datadir", help="The directory storing data", required=True)
    parser.add_argument("--testfolds", help="The total number of folds used in the experiment", required=True)
    args = parser.parse_args()
    return args


def GetToolchainInfo(parts):
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    return parts[i - 1], parts[i], parts[i + 1], parts[i + 2]
    assert(0 and "Should not be here!")
    return None

def GetPredictionResults(fold):
    fileName = os.path.join(args.predictdir, "out.{0}".format(fold))
    f = open(fileName, "r")
    contents = f.read()
    contents = contents.split("\n")[:-1]
    results = []
    for line in contents:
        if len(line) == 0: continue
        results.append(line.split()[0])
    f.close()
    return results

def GetLabels(fold):    
    labels = []
    for fileline in open(os.path.join(args.datadir, "{0}.test".format(fold)), "r"):
        parts = fileline[:-1].split("/")
	filename = parts[-1]
	proj, compiler, version, opt = GetToolchainInfo(parts)
	prefix = os.path.join(args.datadir , "{0}_{1}_{2}_{3}_{4}".format(proj, compiler, version, opt, filename))
	for line in open(prefix + ".data", "r"):
	    if len(line) < 2: continue
	    labels.append(line.split("\t")[0])
    return labels


def Average(folds):
    cor = 0
    tot = 0
    for i in folds:
        cor += results[i][0]
	tot += results[i][1]
    print "Average {0}/{1} = {2}%,".format(cor, tot, cor * 100.0 / tot)

args = getParameters()

testFolds = int(args.testfolds)
results = []
confusionMatrix = {}
for fold in range(testFolds):
    predictLabels = GetPredictionResults(fold)
    groundTruth = GetLabels(fold)
    assert(len(predictLabels) == len(groundTruth))
    cor = 0
    for i in range(len(groundTruth)):
        if predictLabels[i] == groundTruth[i]:
	    cor += 1
	g = int(groundTruth[i])
	p = int(predictLabels[i])
	if g not in confusionMatrix:
	    confusionMatrix[g] = {}
	if p not in confusionMatrix[g]:
	    confusionMatrix[g][p] = 0
	confusionMatrix[g][p] += 1
    if len(groundTruth) == 0:
        print "Fold {0} Empty".format(fold)
	results.append( (0,0) )
	continue
    print "Fold {0}: {1}/{2} = {3}%,".format(fold, cor, len(groundTruth), cor * 100.0 / len(groundTruth))
    results.append( (cor, len(groundTruth) ) )

Average(range(0, testFolds))
for i in range(1, 1+ len(confusionMatrix)):
    line = ""
    for j in range(1, 1+len(confusionMatrix[i])):
        if j not in confusionMatrix[i]:
	    confusionMatrix[i][j] = 0
	line += "\t{0}".format(confusionMatrix[i][j])
    print line

