#!/bin/bash
source config.sh
source scripts/config-bench.sh

CPUS=(24)
PERCENTAGES=(60 65 70 75 80 85 90 100)
BENCHS=(memcached) 
TEST_SYSTEMS=(hermit)
NET_LAT=130
OUTPUT_FILE=$OUT_PATH/memcached/memcached-a.txt
touch $OUTPUT_FILE
for S in ${TEST_SYSTEMS[@]}; do
    DIR=$1
    echo ${DIR}
    for C in ${CPUS[@]}; do
        echo "$S $C" | tee -a $OUTPUT_FILE
        TMP_RESULTS=""
        for P in ${PERCENTAGES[@]}; do
            LAT=`grep "p99-latency:" $DIR/${P}.txt | awk '{print $2}'`
            LAT=$(python3 -c "print($LAT + $NET_LAT)")
            PERCENT=${P}

            TMP_RESULTS+="$PERCENT $LAT\n"
        done
        FIRST_LINE=$(printf "%b" "$TMP_RESULTS" | head -n 1)
        read -r FIRST_PERCENT FIRST_LAT <<< "$FIRST_LINE"
        NEW_PERCENT=$(python3 -c "print($FIRST_PERCENT - 1)")
        NEW_LAT=$(python3 -c "print($FIRST_LAT + 300)")
        TMP_RESULTS="$NEW_PERCENT $NEW_LAT\n$TMP_RESULTS"
        printf "%b" "$TMP_RESULTS" | tee -a $OUTPUT_FILE
        echo "" | tee -a $OUTPUT_FILE
        echo "" | tee -a $OUTPUT_FILE
    done
done
