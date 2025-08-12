#!/bin/bash
source config.sh
source scripts/config-bench.sh

set -euo

echo $MEM_EXTRA_MB

OUT_PATH="$OUT_PATH/microbench/${DATE}"
#MEMORIES=(6400 5760 5120 4480 3840 3200 2560 1920 1280 640)
MEMORIES=(2048)
#MEMORIES=(${FULL_MB})
TRIES=(1)
STRIDES=(2)

FULL_CLEAN=n

export CPUS=(1 2 4 8 16 24 28 32 40 48)
#export CPUS=(1 2)

mkdir -p $OUT_PATH
pushd ${ROOT_PATH}/apps/microbench
make clean
make -j$(nproc) || exit 1
popd

ENABLE_PARSE=y
for TRY in ${TRIES[@]}; do
for S in "${STRIDES[@]}"; do
for M in ${MEMORIES[@]}; do
    for C in ${CPUS[@]}; do
	FILE_OUT="${OUT_PATH}/microbench-$M-$C-$TRY-$S.txt"
	echo ${FILE_OUT}
	sleep 1
	echo $(($M*1024*1024)) > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes
	#Need to check whether it works
	#cgexec --sticky -g memory:bench /home/yupan/hermit/sys_test/page_fault -t $C -d -s $S  | tee ${FILE_OUT}
	cgexec --sticky -g memory:bench ./apps/microbench/page_fault -t $C -d -s $S | tee ${FILE_OUT}
    done
done
done
done

echo "Result in: "
echo ${OUT_PATH}

if [[ ${ENABLE_PARSE} == "y" ]]; then
	./scripts/parse-microbench-hermit.sh $OUT_PATH
else
	echo "Skipping parsing"
fi