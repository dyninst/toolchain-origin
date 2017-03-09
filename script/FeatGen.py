import sys
import argparse
import os;
from subprocess import *

binFeat = "/nobackup/xmeng/toolchain-authorship/toolchain-identification/feature-extraction/func-features/binFeat"
compilerList = ["GCC", "ICC", "LLVM", "PGI"]

def getParameters():
    parser = argparse.ArgumentParser(description='Extract function level code features for toolchain identification')
    parser.add_argument("--filelist", help="A list of binaries to extract features", required=True)
    parser.add_argument("--outputdir", help="The directory to store extracted features", required = True)
    parser.add_argument("--idiom", help="Extract instruction idioms with specified sizes")
    parser.add_argument("--graphlet", help="Extract graphlets for functions")

    args = parser.parse_args()
    return args 

def ParseFeatureSize(param):
    if param == None:
        return None
    ret = []
    for size in param.split(":"):
        ret.append(int(size))
    return ret

def Execute(featType, featSize, path, filename):
    global featureDir
    out = os.path.join(featureDir, "{0}.{1}.{2}".format(filename, featType, featSize))
    cmd = "{0} {1} {2} {3} {4}".format(binFeat, path, featType, featSize, out)
    print cmd
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    msg, err = p.communicate()
    if (len(err) > 0):
        print "Error message:", err
 

def GenerateFeatures(featType, featSizes, path, filename):
    if featSizes == None:
        return
    if featType == "libcall" or featType == "insns_freq":
        Execute(featType, 1, path, filename)
    else:
        for size in featSizes:
	    Execute(featType, size, path, filename)

def BuildDirStructure(featRootDir, dirs):
    curDir = os.getcwd()
    os.chdir(featRootDir)
    for d in dirs:
        if not os.path.exists(d):
	    os.makedirs(d)
	os.chdir(d)
    os.chdir(curDir)

def GenerateDirPath(parts):
    # the output directory should have the following structure
    # featureRootDir/proj/compiler/version/opt
    compiler = "None"
    for i in range(len(parts)):
        if parts[i] in compilerList:
	    compiler = parts[i]
	    version = parts[i+1]
	    opt = parts[i+2]
	    proj = parts[i-1]
	    break
    assert(compiler != "None")
    BuildDirStructure(args.outputdir, [proj, compiler, version, opt])
    return os.path.join(args.outputdir, proj, compiler, version, opt)

args = getParameters()
idiomSizes = ParseFeatureSize(args.idiom)
graphletSizes = ParseFeatureSize(args.graphlet)
for line in open(args.filelist, "r"):
    path = line[:-1]
    parts = path.split("/")
    featureDir = GenerateDirPath(parts)
    filename = path.split("/")[-1]
    GenerateFeatures("idiom", idiomSizes, path, filename)
    GenerateFeatures("graphlet", graphletSizes, path, filename)

