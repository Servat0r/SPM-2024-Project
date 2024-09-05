#!/bin/bash

# Define parameter ranges or lists
policy_list=(1 2 3)
ntasks_list=(1 2 4 6 8 10 12 14 16)
tileSize_list=(1 4 8)
chunkSize_list=(128)
MAXWORKERS=16 # Maximum amount of spawnable workers

if [ $# -lt 1 ]; then
  echo "Usage: ffruns_spmcluster.sh N"
  exit 1
fi

N=$1
echo "Using a matrix of size $N ..."

# Sequential Runs
for tileSize in "${tileSize_list[@]}"; do
    echo "Sequential run with parameters: N=$N, ntasks=1, tileSize=$tileSize"
    srun --nodes 1 ./UTWavefrontFF $N 0 $tileSize 1 128 $MAXWORKERS output_results_ff_spmcluster_${N}size.csv
    if [ $? -ne 0 ]; then
        echo "An error occurred with parameters: N=$N, policy=0, nnodes=1, ntasks=1, tileSize=$tileSize, chunkSize=128"
        read -p "Continue execution (y/n)?" cont
        if [ "$cont" == "n" ]; then
            break 1  # Only for bash 4 or above
        fi
    fi
done

# Loop over all combinations of parameters for Parallel Runs
for policy in "${policy_list[@]}"; do
    for ntasks in "${ntasks_list[@]}"; do
        for tileSize in "${tileSize_list[@]}"; do
            for chunkSize in "${chunkSize_list[@]}"; do
                echo "Running with parameters: N=$N, policy=$policy, ntasks=$ntasks, tileSize=$tileSize, chunkSize=$chunkSize"
                srun --nodes 1 ./UTWavefrontFF $N $policy $tileSize $ntasks $chunkSize 16 output_results_ff_spmcluster_${N}size.csv
                if [ $? -ne 0 ]; then
                    echo "An error occurred with parameters: N=$N, policy=$policy, nnodes=$nnodes, ntasks=$ntasks, tileSize=$tileSize, chunkSize=$chunkSize"
                    read -p "Continue execution (y/n)?" cont
                    if [ "$cont" == "n" ]; then
                        break 4  # Only for bash 4 or above
                    fi
                fi
            done
        done
    done
done
