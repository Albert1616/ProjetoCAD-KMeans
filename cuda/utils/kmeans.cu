#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "aluno.h"
#include "kmeans.h"

#define MAX_K 1000

__device__ float distEuclidiana(Aluno *a, Aluno *b) {
    float diffMedia = a->media - b->media;
    float diffFaltas = a->numeroFaltas - b->numeroFaltas;
    return sqrtf(diffMedia * diffMedia + diffFaltas * diffFaltas);
}

__global__ void initCentroids_kernel(KMeans *model, Aluno *alunos) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        int listIndex[MAX_K];
        for (int i = 0; i < model->k; i++) listIndex[i] = -1;

        for (int i = 0; i < model->k; i++) {
            unsigned int index = (model->random_state + i * 1103515245U) % model->totalAlunos;            
            int duplicado = 0;
            for (int j = 0; j < i; j++) {
                if (listIndex[j] == index) { duplicado = 1; break; }
            }
            if (duplicado) { i--; continue; }
            listIndex[i] = index;
            model->centroids[i] = alunos[index];
        }
    }
}

__global__ void assignClusters_kernel(KMeans *model, Aluno *alunos) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < model->totalAlunos) {
        float min_dist = 1e9f;
        int best_cluster = 0;

        for (int j = 0; j < model->k; j++) {
            float dist = distEuclidiana(&alunos[id], &model->centroids[j]);
            if (dist < min_dist) {
                min_dist = dist;
                best_cluster = j;
            }
        }
        alunos[id].cluster = best_cluster;
    }
}

__global__ void resetCentroids_kernel(Aluno *new_centroids, int *counts, int k) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < k) {
        new_centroids[id].media = 0.0f;
        new_centroids[id].numeroFaltas = 0.0f;
        counts[id] = 0;
    }
}

__global__ void updateCentroids_kernel(Aluno *alunos, Aluno *new_centroids, int *counts, int totalAlunos) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < totalAlunos) {
        int c = alunos[id].cluster;
        atomicAdd(&new_centroids[c].media, alunos[id].media);
        atomicAdd(&new_centroids[c].numeroFaltas, alunos[id].numeroFaltas);
        atomicAdd(&counts[c], 1);
    }
}

__global__ void averageAndCheck_kernel(KMeans *model, Aluno *new_centroids, int *counts, int *d_convergiu) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < model->k) {
        if (counts[id] > 0) {
            new_centroids[id].media /= counts[id];
            new_centroids[id].numeroFaltas /= counts[id];
        }

        float dist = distEuclidiana(&model->centroids[id], &new_centroids[id]);
        if (dist > 0.0001f) {
            *d_convergiu = 0;
        }

        model->centroids[id].media = new_centroids[id].media;
        model->centroids[id].numeroFaltas = new_centroids[id].numeroFaltas;
    }
}

__global__ void predict(KMeans *model, Aluno *novoAluno) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id == 0) {
        float min_dist = 1e9f;
        int best_cluster = 0;
        for (int i = 0; i < model->k; i++) {
            float dist = distEuclidiana(novoAluno, &model->centroids[i]);
            if (dist < min_dist) {
                min_dist = dist;
                best_cluster = i;
            }
        }
        novoAluno->cluster = best_cluster;
    }
}

__host__ void fit(KMeans *d_model, Aluno *d_alunos, int numIteracoes) {
    KMeans h_model;
    cudaMemcpy(&h_model, d_model, sizeof(KMeans), cudaMemcpyDeviceToHost);

    int k = h_model.k;
    int max_iter = h_model.max_iter;
    int totalAlunos = h_model.totalAlunos;

    int threads = 128;
    int blocosAlunos = (totalAlunos + threads - 1) / threads;
    int blocosK = (k + threads - 1) / threads;

    Aluno *d_new_centroids;
    int *d_counts;
    int *d_convergiu;

    cudaMalloc(&d_new_centroids, k * sizeof(Aluno));
    cudaMalloc(&d_counts, k * sizeof(int));
    cudaMalloc(&d_convergiu, sizeof(int));

    initCentroids_kernel<<<1, 1>>>(d_model, d_alunos);

    for (int iter = 0; iter < numIteracoes; iter++) {
        resetCentroids_kernel<<<blocosK, threads>>>(d_new_centroids, d_counts, k);
        cudaDeviceSynchronize();

        assignClusters_kernel<<<blocosAlunos, threads>>>(d_model, d_alunos);
        cudaDeviceSynchronize();

        updateCentroids_kernel<<<blocosAlunos, threads>>>(d_alunos, d_new_centroids, d_counts, totalAlunos);
        cudaDeviceSynchronize();
    }

    cudaFree(d_new_centroids);
    cudaFree(d_counts);
    cudaFree(d_convergiu);
}
