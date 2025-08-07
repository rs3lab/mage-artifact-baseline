#!/bin/bash
source config.sh
source scripts/config-bench.sh

set -euo

echo $MEM_EXTRA_MB

OUT_PATH="$OUT_PATH/xsbench/${DATE}"
MEMORIES=(15000 13500 12000 10500 9000 7500 6000 4500 3000 1500)

FULL_CLEAN=n

export OMP_CPUS=(48 4)

pushd $ROOT_PATH/apps/XSBench/openmp-threading
make clean
make -j$(nproc) || exit 1
popd

mkdir -p $OUT_PATH

ENABLE_PARSE=y
for M in ${MEMORIES[@]}; do
    for C in ${OMP_CPUS[@]}; do
	FILE_OUT="${OUT_PATH}/xsbench-$M-$C.txt"
	echo ${FILE_OUT}
	sleep 1
	echo $(($M*1024*1024)) > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes
	D=$((C-1))
	#Need to check whether it works
	cgexec --sticky -g memory:bench ${ROOT_PATH}/apps/XSBench/openmp-threading/XSBench -t $C -m history -s XL -l 34 -p 5000000 -G unionized -g 30000 | tee ${FILE_OUT}
    done
done

echo "Result in: "
echo ${OUT_PATH}
if [[ "${ENABLE_PARSE}" == "y" ]]; then
    ./scripts/parse-xsbench-hermit.sh $OUT_PATH
else
    echo "Skipping parsing."
fi