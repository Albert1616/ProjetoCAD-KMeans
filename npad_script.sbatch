#!/bin/bash
#SBATCH --partition gpu-8-v100
#SBATCH --gpus-per-node=1   # Número GPUs por nó
#SBATCH --nodes 1
#SBATCH --time 00:10:00
#SBATCH --job-name kmeans
#SBATCH --output kmeans-%j.out

ulimit -s unlimited
module load compilers/nvidia/nvhpc/24.11

make KmeansOpenMPGPU

make run-omp-gpu
