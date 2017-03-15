#!/bin/sh

installdir=$1
cd liblbfgs-1.10 && ./configure --prefix=$installdir && make && make install
cd ../crfsuite-0.12 && ./configure --prefix=$installdir --with-liblbfgs=$installdir && make && make install
cd ../function-features && make && cp extractFeat $installdir/bin/
cd ../ && cp script/* $installdir/bin
