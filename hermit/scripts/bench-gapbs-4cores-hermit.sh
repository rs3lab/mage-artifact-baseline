#!/bin/bash
source config.sh
source config-bench.sh

set -euo

echo $MEM_EXTRA_MB

OUT_PATH="$OUT_PATH/gapbs/${DATE}"
MEMORIES=(19000 17100 15200 13300 11400 9500 7600 5700 3800 1900)
ALGO=(pr)
GRAPH_TRIAL=3

declare -A ALGO_PARAMS
#ALGO_PARAMS[pr]=" -f /mnt/twitter/twitter.sg -i1000 -t1e-4 "
ALGO_PARAMS[pr]=" -f ${KRON} -i1000 -t1e-4 "

export OMP_CPUS=(4)

pushd $ROOT_PATH/apps/gapbs
make clean
make -j$(nproc) || exit 1
popd

mkdir -p $OUT_PATH

ENABLE_PARSE=y
for A in ${ALGO[@]}; do
    for TRY in ${TRIES[@]}; do
        for M in ${MEMORIES[@]}; do
            for C in ${OMP_CPUS[@]}; do
                FILE_OUT="${OUT_PATH}/gapbs-kron-$A-$M-$C-$TRY.txt"
                echo ${FILE_OUT}
                sleep 1
                echo $(($M*1024*1024)) > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes
                D=$((C-1))
                #Need to check whether it works
                GOMP_CPU_AFFINITY=0-$D OMP_NUM_THREADS=$C cgexec --sticky -g memory:bench ${ROOT_PATH}/apps/gapbs/${A} ${ALGO_PARAMS[$A]} -n${GRAPH_TRIAL} | tee ${FILE_OUT}
            done
        done
    done
done

echo "Result in: "
echo ${OUT_PATH}
if [[ "${ENABLE_PARSE}" == "y" ]]; then
    ./scripts/parse-gapbs-4cores-hermit.sh $OUT_PATH
else
    echo "Skipping parsing."
fi