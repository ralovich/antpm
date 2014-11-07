
PATH=~/bin/cov/cov-analysis-linux64-7.5.0/bin:$PATH

# mkdir build-cov
# (cd build-cov && cmake ../src  -DCMAKE_BUILD_TYPE=Debug -DUSE_BOOST_TEST=TRUE)
# (cd build-cov && cov-build --dir cov-int make -j 9)

COVERITY_SITE_CC=clang;clang++
mkdir build-cov-clang
(cd build-cov-clang && CXX=clang++ CC=clang cmake ../src  -DCMAKE_BUILD_TYPE=Debug -DUSE_BOOST_TEST=TRUE)
(cd build-cov-clang && cov-build --dir cov-int make -j 9)


