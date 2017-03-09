#!/bin/bash
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
tar -zxf input.tar.gz
touch model.dat
python crf_feat_test.py $1 $2 $3 $4
