# Toolchain-origin
This tool uses machine learning to identify the compilation toolchains 
that generated a binary program. The compilation toolchain refers to 
the compiler family, compiler version, and compiler flags that were 
used generated a binary. Toolchain identification is performed at the 
function level, meaning that each function is attributed to a single toolchain.

# Initial support
The current version supports identifying three compiler families 
(GCC, ICC and LLVM) and five optimization flags (-O0, -O1, -O2, 
-O3, and -Os).

# Install

The short version:
1. First edit file ``function-features/Makefile'' and change 
variable DYNINST_DIR to your dyninst install location
2. ./install.sh <path-to-install>

In more details, this toolchain contains three components: 
lbfgs library, crfsuite, and feature extraction.

# Usage

The Origin.py script is given a binary to perform toolchain identification.
For example:

python Origin.py --binpath /home/test/binary \
                 --modeldir /home/new-model \
                 --installdir /home/toolchain-origin \
                 --workingdir /home/tmp

Option "--binpath" specifies the binary to perform toolchain identification.
Option "--modeldir" specifies the model to use. If the option is omitted, the default model
is used. You can follow the steps in "Train your own model" to create your stronger model.
Option "--installdir" specifies the install directory of the tool.
Option "--workingdir" specifies the direcory to store intermediate results,
including the extracted features, the address range of functions in the binary.

# Train your own model

1. Prepare your training binaries

You first need to parepare a text file that contains the full path 
to a binary used in training, one file per line. We use the path structure 
to encode the toolchain ground truth. For example, file path

/home/training/dyninst/GCC/4.8.5/O3/lib/libdyninstAPI.so

means that this binary is compiled with GCC, version 4.8.5, with optimization level -O3.
The specific requirement is that the file path must contains 

"software-name/compiler-name/compiler-version/opt-level/".

In the above example, software-name is dyninst; compiler-name is GCC; compiler-version is 4.8.5;
and opt-level is O3. Note that it is OK to have subdirectory after the opt-level directory.

So, your training directory should look like:

/home/training - /dyninst - /GCC - /4.8.5 - /O0 ...
                                          - /O1 ...
                                          - /O2 ...
                                          ...
                                   /5.4.3 - /O0 ...
                                  ...
                            /ICC - /13.0.0 - /O0 ...
                            ...
                 /apache-httpd - /GCC ...
                               - /LLVM ...
                                 ...
                 /binutil ...

At the end of the step, you will have a file to describe all the binaries for training. 
For example, /home/training/filelist.txt

2. Extract code features
In this step, you need to use the script named "FeatGen.py", 
which will iteratively extract code features from every binary in your training set.
An example of running the script is as follow:

python FeatGen.py --filelist /home/training/filelist.txt \ 
                  --idiom 1:2:3 \
                  --graphlet 1:2:3 \
                  --path_to_extract_bin /home/toolchain-origin-install/bin/extractFeat \
                  --outputdir /home/features


Option "--filelist" specifies the full path to the file list prepared in step 1.
Option "--outputdir" specifies the diretory to store the extracted features. 
The script will store extracted features in a direcotry structure similar to the one
you prepread for trainiing binaries. So, you will have a feature directory looking like

/home/features - /dyninst - /GCC - /4.8.5 - /O0 ...
                                          - /O1 ...
                                          - /O2 ...
                                          ...
                                   /5.4.3 - /O0 ...
                                   ...
                            /ICC - /13.0.0 - /O0 ...
                            ...
                 /apache-httpd - /GCC ...
                               - /LLVM ...
                                 ...
                 /binutil ...

Option "--idiom" specifies the sizes of instruction idiom features to extract. Sizes are colon seperated. 
Option "--graphlet" specifies the sizes of graphlet features to extract. Size are colon seperated. 
So, "1:2:3" means extracting features in size 1, 2, and 3. And "1:2:3" is a commonly use combination.
Option "--path_to_extract_bin" specifies the full path to the executable that extracts features, which 
should be in the install directory of toolchain-origin.

3. Generate training data

In this step, you use the script named "GenDataFiles.py", which generates a global feature index.
An example of runing the script is as follow:

python GenDataFiles.py --filelist /home/training/filelist.txt \
                       --idiom 1:2:3 \
                       --graphlet 1:2:3 \
                       --featdir /home/features \
                       --outputdir /home/training-data

Option "--filelist" specifies a file list, which you want to use to generate the training data. Note that
you can use the same file list as the previous step. But you can also use a subset of files.
Option "--idiom" and "--graphlet" specifies the feature sizes.
Option "--featdir" specifies the directory storing the extracted features, which should be the output directory
of step 2.

In the output directory, there are two interesting files:
feat_list.txt, in which each line represents a feature,
and toolchain-index.txt, in which each line represents a toolchain label.

4. Perform feature selection

In this step, you use the script named "FS.py", which generates a file contains the top features
that are ranked by the mutual information between the feature and the toolchain labels.

python FS.py --filelist /home/training/filelist.txt \ 
             --datadir /home/training-data \
             --keep 2000 \ 
             --output /home/training-data/fs.txt

In the output file, each line represents a feature and features are ordered by their mutual information
with the toolchain labels. So, the first line is the feature that has the most mutual information.
Each line contains the feature index, the feature string representation, and the scale of the feauture 
to make it distributed between 0 and 1 in the training set.

5. Train the model

In this step, you use the script named "TrainModel.py", which trains a linear conditional random field model 
for toolchain identification.

python TrainModel.py --filelist /home/training/filelist.txt \
                     --datadir /home/training-data \
                     --featurelist /home/training-data/fs.txt \
                     --path_to_crfsuite /home/toolchain-origin-install/bin/crfsuite

The current directory will have a file named "model.dat", which is the new model.

6. Put the model and other needed data into a new directory

You need to copy or move three files (the new model, feature list, and toolchain label)
to a new directory:

mv model.dat /home/new-model/model.dat
mv /home/training-data/fs.txt /home/new-model/features.txt
mv /home/training-data/toolchain-index.txt /home/new-model/toolchain-index.txt

You can use the new model by feeding the directory "/home/new-model" as
the "--modeldir" option of the Origin.py script.
