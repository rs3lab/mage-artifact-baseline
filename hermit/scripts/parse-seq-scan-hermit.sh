#!/bin/bash
source config.sh
source scripts/config-bench.sh

CPUS=(48)
TRY=1
TRIES=$(seq 1 ${TRY})
MEMORIES=(6400 5760 5120 4480 3840 3200 2560 1920 1280 640)
PREFETCHERS=(readahead no)
STRIDES=(1 2)
MAX_MEMORY=${MEMORIES[0]}
TEST_SYSTEM=hermit

OUTPUT_FILE=$OUT_PATH/seq_scan/seq_scan.txt
touch $OUTPUT_FILE
DIR=$1
echo ${DIR}
for C in ${CPUS[@]}; do
    for i in "${!STRIDES[@]}"; do
        S=${STRIDES[$i]}
        P=${PREFETCHERS[$i]}
        echo "$TEST_SYSTEM $C $P" | tee -a $OUTPUT_FILE
        for M in ${MEMORIES[@]}; do
            RESULT=0
            for T in ${TRIES[@]}; do
                TMP=`grep tput $DIR/seq-scan-$M-$C-$T-$STRIDE.txt | head -n 1| awk '{print $2}'`
                echo $DIR 
                echo $TMP
                if [[ -z $TMP ]]; then
                    TMP=0
                fi
                RESULT=$(python3 -c "print($RESULT + $TMP)")
            done
            if [[ "$RESULT" != "0" ]]; then
                RESULT=$(python3 -c "print(round($RESULT / $TRY, 2))")
            fi
            PERCENT=$(python3 -c "print(int(100 * $M/$MAX_MEMORY))")
            echo "$PERCENT $RESULT" | tee -a $OUTPUT_FILE
        done
        echo "" | tee -a $OUTPUT_FILE
        echo "" | tee -a $OUTPUT_FILE
    done
done