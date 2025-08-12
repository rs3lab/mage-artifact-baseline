#!/bin/bash
source config.sh
source scripts/config-bench.sh

set -euo

echo $MEM_EXTRA_MB

OUT_PATH="$OUT_PATH/seq_scan/${DATE}"
MEMORIES=(6400 5760 5120 4480 3840 3200 2560 1920 1280 640)
TRIES=(1)
STRIDES=(1 2)

FULL_CLEAN=n

export CPUS=(48)

pushd ${ROOT_PATH}/apps/seq_scan
make clean
make -j$(nproc) || exit 1
popd

mkdir -p $OUT_PATH

ENABLE_PARSE=y
for TRY in ${TRIES[@]}; do
	for S in "${STRIDES[@]}"; do
		for M in ${MEMORIES[@]}; do
			for C in ${CPUS[@]}; do
			FILE_OUT="${OUT_PATH}/seq-scan-$M-$C-$TRY-$S.txt"
			echo ${FILE_OUT}
			sleep 1
			echo $(($M*1024*1024)) > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes
			cgexec --sticky -g memory:bench ${ROOT_PATH}/apps/seq_scan/seq_scan -t $C -d -s $S -w  | tee ${FILE_OUT}
			done
		done
	done
done

echo "Result in: "
echo ${OUT_PATH}

if [[ ${ENABLE_PARSE} == "y" ]]; then
	./scripts/parse-seq-scan-hermit.sh $OUT_PATH
else
	echo "Skipping parsing"
fi