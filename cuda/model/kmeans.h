#ifndef KMEANS_H
#define KMEANS_H
#include "aluno.h"

typedef struct
{
    int k;
    int max_iter;
    int random_state;
    int totalAlunos;
    Aluno *centroids;
} KMeans;

#ifdef __CUDACC__
__global__ void initCentroids(KMeans *model, Aluno *alunos);
__global__ void assignClusters(KMeans *model, Aluno *alunos);
__global__ void updateCentroids(KMeans *model, Aluno *alunos);
__global__ void resetCentroids_kernel(Aluno *new_centroids, int *counts, int k);
__global__ void averageAndCheck_kernel(KMeans *model, Aluno *new_centroids, int *counts, int *d_convergiu);
__global__ void predict(KMeans *model, Aluno *novoAluno);
__host__ void fit(KMeans *model, Aluno *alunos, int numIteracoes);
__host__ float *methodElbow(KMeans *h_model, KMeans *d_model, Aluno *h_alunos, Aluno *d_alunos);
#endif

#endif // KMEANS_H
