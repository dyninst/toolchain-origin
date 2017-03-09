# See README.txt for the details of the experiments

import sys
import os
import math
import argparse
from subprocess import *

def getParameters():
    parser = argparse.ArgumentParser(description='Generate submission files of HTCondor to run experiments to select features')
    parser.add_argument("--testfolds", help="The number of folds used in the experiment to evaluate accuracy", required=True)
    parser.add_argument("--fsdir", help ="The root directory to store condor related files", required=True)
    parser.add_argument("--keep", help="The maximal number of features to keep", required= True)
    args = parser.parse_args()
    return args

def GenJobDescription(test, keep, f,  testFolds):
    f.write("output = fs{0}.txt\n".format(test))
    f.write("error = error.{0}\n".format(test))
    f.write("log = log.{0}\n".format(test))
    f.write('arguments = "{0} {1} {2}"\n'.format(testFolds, test,keep))
    f.write("queue\n")


args = getParameters()
testFolds = int(args.testfolds)
keep = int(args.keep)

submitFileName = os.path.join(args.fsdir, "submit_fs.txt")
f = open(submitFileName, "w")
f.write("universe = vanilla\n")
f.write("should_transfer_files = YES\n")
f.write("executable = ../../script/FS.sh\n")
f.write("notification = Never\n")
f.write("request_disk = 40 GB\n")
f.write("request_memory = 8 GB\n")
f.write('Requirements = ( ( OpSysAndVer == "RedHat6" ) ) || ( ( OpSysAndVer == "SL6" ) )\n')
f.write("transfer_input_files = ../input.tar.gz, ../../script/FS.py\n")

total = 0
# Generate submit files
for test in range(testFolds):
    GenJobDescription(test, keep, f, testFolds)

f.close()
print "All done"
