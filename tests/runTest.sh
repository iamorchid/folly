#!/bin/bash

# build folly 命令 (指定build的输出路径)
# python3 ./build/fbcode_builder/getdeps.py --allow-system-packages --scratch-path /Users/will/workspace/folly_build  build 

FOLLY_INSTALLED_DIR=/Users/will/workspace/folly_build/installed

set -e 

SCRIPT_DIR=$(dirname "$(realpath "$0")")

c++ -std=c++17 \
  -Wno-unqualified-std-cast-call \
  -Wno-tautological-pointer-compare \
  -l c++abi -l event -l jemalloc -l double-conversion -l boost_context-mt \
  -I ${FOLLY_INSTALLED_DIR}/folly/include \
  -L ${FOLLY_INSTALLED_DIR}/folly/lib \
  -l folly \
  -I ${FOLLY_INSTALLED_DIR}/fmt--fKE9t5lFonrZffRLwaFRyfNw2oEKmpDdVCtWHHzyqs/include \
  -L ${FOLLY_INSTALLED_DIR}/fmt--fKE9t5lFonrZffRLwaFRyfNw2oEKmpDdVCtWHHzyqs/lib \
  -l fmt \
  -I ${FOLLY_INSTALLED_DIR}/glog-v_HyOTZ8X1S8nKI0Fu0gSL6W2whU5ESdMXbDY6BPJOI/include \
  -Wl,-rpath,${FOLLY_INSTALLED_DIR}/glog-v_HyOTZ8X1S8nKI0Fu0gSL6W2whU5ESdMXbDY6BPJOI/lib \
  -L ${FOLLY_INSTALLED_DIR}/glog-v_HyOTZ8X1S8nKI0Fu0gSL6W2whU5ESdMXbDY6BPJOI/lib \
  -l glog \
  -I ${FOLLY_INSTALLED_DIR}/gflags-jv6n8xt7zUWG16QHG40g1bLfE6SWZGYB6Ocy3RFqypA/include \
  -Wl,-rpath,${FOLLY_INSTALLED_DIR}/gflags-jv6n8xt7zUWG16QHG40g1bLfE6SWZGYB6Ocy3RFqypA/lib \
  -L ${FOLLY_INSTALLED_DIR}/gflags-jv6n8xt7zUWG16QHG40g1bLfE6SWZGYB6Ocy3RFqypA/lib \
  -l gflags \
  -o ${SCRIPT_DIR}/bin/test \
  "$@"

${SCRIPT_DIR}/bin/test