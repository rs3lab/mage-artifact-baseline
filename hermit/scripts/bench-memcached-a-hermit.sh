#!/bin/bash
source config.sh

DATE=$(date +"%m.%d:%H.%M")

OUT_PATH="~/benchmark-out-ae/memcached/expa/${DATE}"
TOTAL_MEM=20285996540 # 20 gb

PERCENTAGES=(100 90 85 80 75 70 65 60)

BENCH="./memcached"
stime=0

pushd $SOURCE_DIR
mkdir -p $OUT_PATH

ENABLE_PARSE=y
for percent in ${PERCENTAGES[@]}; do
    
    make clean
    stime=0 make         
    
    echo "Running for time: $stime"
    M=$(echo "$TOTAL_MEM * $percent / 100" | bc)
        
    # Limit the cgroup
    echo $M > /sys/fs/cgroup/memory/bench/memory.limit_in_bytes

    sleep 1
    cgexec --sticky -g memory:bench $BENCH -u root -t 12 -m200000 | tee "$OUT_PATH/$percent.txt"
done
popd

echo "Result in: "
echo ${OUT_PATH}
if [[ "${ENABLE_PARSE}" == "y" ]]; then
    ./scripts/parse-memcached-a-hermit.sh $OUT_PATH
else
    echo "Skipping parsing."
fi