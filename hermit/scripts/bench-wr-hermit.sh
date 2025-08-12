#!/bin/bash
source config.sh
source scripts/config-bench.sh

set -euo

echo $MEM_EXTRA_MB

OUT_PATH="$OUT_PATH/wrmem/${DATE}"
MEMORIES=(30000 27000 24000 21000 18000 15000 12000 9000 6000 3000)
TRIES=(1)

FULL_CLEAN=n

export CPUS=(48 4)

mkdir -p $OUT_PATH

APP_DIR="${ROOT_PATH}/apps/metis/"
pushd $APP_DIR
make clean
make
popd
ENABLE_PARSE=y
for TRY in ${TRIES[@]}; do
for M in ${MEMORIES[@]}; do
    for C in ${CPUS[@]}; do
	FILE_OUT="${OUT_PATH}/wrmem-$M-$C-$TRY.txt"
	echo ${FILE_OUT}
	sleep 1
	echo $(($M*1024*1024)) > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes
	#Need to check whether it works
	cgexec --sticky -g memory:bench ./apps/metis/obj/app/wrmem -s 5000 -p${C} | tee ${FILE_OUT}
    done
done
done

echo "Result in: "
echo ${OUT_PATH}
if [[ "${ENABLE_PARSE}" == "y" ]]; then
	./scripts/parse-wr-hermit.sh $OUT_PATH
else
	echo "Skipping parsing."
fi