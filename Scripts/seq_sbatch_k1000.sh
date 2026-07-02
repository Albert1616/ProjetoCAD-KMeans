#!/bin/bash
#SBATCH --partition amd-512
#SBATCH --nodes 1
#SBATCH --cpus-per-task=32
#SBATCH --time 10:00:00
#SBATCH --job-name kmeans-seq

ulimit -s unlimited
module load compilers/nvidia/nvhpc/24.11

mkdir -p output_logs
LOG_FILE="output_logs/seq_results.csv"
for K in 1000; do
    echo "================================================="
    echo "[AUDIT] PREPARANDO SEQUENCIAL PARA K=$K"
    echo "================================================="
    sed -i "s/#define NUM_CLUSTERS .*/#define NUM_CLUSTERS $K/g" KmeansSequencial.c
    make clean > /dev/null
    make KmeansSequencial > /dev/null

    echo ">>> EXPERIMENTO 1: FRACA (VARIANDO ALUNOS) K=$K <<<"
    for ALUNOS in 20000 40000 80000 160000 320000 640000 1000000; do
        echo "[AUDIT] Rodando Sequencial Alunos=$ALUNOS, K=$K, MAX_ITER=10000, FIT=10, RANDOM_STATE=42, THREAD=1"
        seq_time=$(./KmeansSequencial $ALUNOS 10 | tee /dev/stderr | grep "Duração do treinamento" | awk '{print $4}')
        echo "fraca,$K,$ALUNOS,1,$seq_time" >> $LOG_FILE
    done
done
