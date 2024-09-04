#!/bin/bash

# Define parameter ranges or lists
policy_list=(1 2 3)
ntasks_list=(1 2 4 6 8 10 12 14 16) # Number of tasks + frontend task
tileSize_list=(1 4 8)
chunkSize_list=(128)

read -p "Enter the size of the matrix to use: " N
echo "Using a matrix of size $N ..."

# Loop over all combinations of parameters
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
