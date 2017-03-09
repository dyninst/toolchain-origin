#!/bin/bash
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
tar -zxf input.tar.gz
touch tmp.svm.model
python svm_feat_test.py $1 $2 $3 $4
