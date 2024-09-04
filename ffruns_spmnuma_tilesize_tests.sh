#!/bin/bash

# Define parameter ranges or lists
policy_list=(1)
ntasks_list=(1 2 4 6 8 10 12 14 16 20 24 28 32) # Number of tasks + frontend task
tileSize_list=(16 32 64 128)
chunkSize_list=(128)
N=6000

# Loop over all combinations of parameters
for policy in "${policy_list[@]}"; do
    for ntasks in "${ntasks_list[@]}"; do
        for tileSize in "${tileSize_list[@]}"; do
            for chunkSize in "${chunkSize_list[@]}"; do
                echo "Running with parameters: N=$N, policy=$policy, ntasks=$ntasks, tileSize=$tileSize, chunkSize=$chunkSize"
                ./UTWavefrontFF $N $policy $tileSize $ntasks $chunkSize 32 output_results_ff_spmnuma_${N}size_tilesize_tests.csv
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
