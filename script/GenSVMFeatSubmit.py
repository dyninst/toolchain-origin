# See README.txt for the details of the experiments

import sys
import os
import math
import argparse
from subprocess import *

libLinearTrain = "../../../learning/train"
libLinearPredict = "../../../learning/predict"

featIndex = {}
featList = []
totalFeat = 0

def getParameters():
    parser = argparse.ArgumentParser(description='Generate submission files of HTCondor to run experiments to evaluate the accuracy of a learning algorithm')
    parser.add_argument("--testfolds", help="The number of folds used in the experiment to evaluate accuracy", required=True)
    parser.add_argument("--submitfile", help ="The path to the submit file", required=True)
    parser.add_argument("--keep", help="How many feature to keep")
    parser.add_argument("--featdir", help="Feature file")
    args = parser.parse_args()
    return args

def GenJobDescription(test, keep, f, total, testFolds):
    featureFile = "{0}/{1}/fs{2}.txt".format(expDir, args.featdir,test)
    if len(open(featureFile, "r").read()) < 50: return

    f.write("transfer_input_files = ../input.tar.gz, ../../script/svm_feat_test.py, ../{0}/fs{1}.txt,{2},{3}\n".format(args.featdir, test, libLinearTrain, libLinearPredict))
    f.write("output = out.{0}\n".format(total))
    f.write("error = error.{0}\n".format(total))
    f.write("log = log.{0}\n".format(total))
    f.write('arguments = "{0} {1} {2} {3}"\n'.format(testFolds, test, keep, total))
    f.write("queue\n")


args = getParameters()
testFolds = int(args.testfolds)
keep = int(args.keep)
expDir = "/".join(args.submitfile.split("/")[:-2])
f = open(args.submitfile, "w")
f.write("universe = vanilla\n")
f.write("should_transfer_files = YES\n")
f.write("executable = ../../script/svm_feat_test.sh\n")
f.write("notification = Never\n")
f.write("request_disk = 40 GB\n")
f.write("request_memory = 8 GB\n")
f.write('Requirements = ( ( OpSysAndVer == "RedHat6" ) ) || ( ( OpSysAndVer == "SL6" ) )\n')
f.write("transfer_output_files = tmp.svm.model\n")
total = 0 

for test in range(testFolds):
    GenJobDescription(test,  keep, f, total, testFolds)
    total += 1

f.close()
print "All done"
