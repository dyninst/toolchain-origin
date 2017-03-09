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
    parts = contents.split("Item accuracy: ")[1].split()
    c = int(parts[0])
    t = int(parts[2])
    f.close()
    return c, t

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
cor = 0
tot = 0
for fold in range(testFolds):
    c, t = GetPredictionResults(fold)
    cor += c
    tot += t
    print "Fold {0}: {1}/{2} = {3}%,".format(fold, c, t, c * 100.0 / t)

print "Overall: {0}/{1}={2}%".format(cor, tot, cor * 100.0 / tot)
