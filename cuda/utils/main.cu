#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cuda_runtime_api.h>
#include "aluno.h"
#include "dataset.h"
#include "kmeans.h"

#define NUM_CLUSTERS 3
#define MAX_ITERATIONS 10000
#define NUM_ALUNOS 999999
#define NUM_FIT_ITERATIONS 2000

void exportarResultados(Aluno *alunos, int total)
{
    FILE *file = fopen("resultados.csv", "w");
    fprintf(file, "numeroFaltas,media,cluster\n");

    for (int i = 0; i < total; i++)
        fprintf(file, "%.2f,%.2f,%d\n",
                alunos[i].numeroFaltas,
                alunos[i].media,
                alunos[i].cluster);

    fclose(file);
}

int main()
{
    cudaEvent_t start, stop;
    float tempo_gasto_ms = 0;

    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    Aluno *h_alunos = (Aluno *)malloc(NUM_ALUNOS * sizeof(Aluno));
    int numeroAlunos = carregarDataset(h_alunos, NUM_ALUNOS);

    normalizarAlunos(h_alunos, numeroAlunos);

    cudaEventRecord(start);
    // aloca e copia alunos para GPU
    Aluno *d_alunos;
    cudaMalloc(&d_alunos, numeroAlunos * sizeof(Aluno));
    cudaMemcpy(d_alunos, h_alunos, numeroAlunos * sizeof(Aluno), cudaMemcpyHostToDevice);

    printf("Configurar modelo");

    // configura modelo na CPU
    KMeans h_kmeans;
    h_kmeans.k = NUM_CLUSTERS;
    h_kmeans.max_iter = MAX_ITERATIONS;
    h_kmeans.random_state = 42;
    h_kmeans.totalAlunos = numeroAlunos;
    h_kmeans.centroids = (Aluno *)malloc(h_kmeans.k * sizeof(Aluno));

    // aloca centroids na GPU e atualiza ponteiro no modelo
    Aluno *d_centroids;
    cudaMalloc(&d_centroids, h_kmeans.k * sizeof(Aluno));

    // copia modelo para GPU
    KMeans *d_kmeans;
    cudaMalloc(&d_kmeans, sizeof(KMeans));
    cudaMemcpy(d_kmeans, &h_kmeans, sizeof(KMeans), cudaMemcpyHostToDevice);

    // corrige ponteiro de centroids dentro do d_kmeans
    cudaMemcpy(&(d_kmeans->centroids), &d_centroids, sizeof(Aluno *), cudaMemcpyHostToDevice);

    // treinamento: A CPU chama a função fit para orquestrar os kernels
    fit(d_kmeans, d_alunos, NUM_FIT_ITERATIONS);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&tempo_gasto_ms, start, stop);

    // traz resultados para CPU
    cudaMemcpy(h_alunos, d_alunos, numeroAlunos * sizeof(Aluno), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_kmeans.centroids, d_centroids, h_kmeans.k * sizeof(Aluno), cudaMemcpyDeviceToHost);

    exportarResultados(h_alunos, numeroAlunos);

    for (int i = 0; i < h_kmeans.k; i++)
        printf("Centroid %d - Media: %.2f, Numero de Faltas: %.2f\n", i,
               h_kmeans.centroids[i].media, h_kmeans.centroids[i].numeroFaltas);

    // predição
    Aluno novoAluno;
    novoAluno.media = 0.95;
    novoAluno.numeroFaltas = 0.1;

    printf("Tempo total: %.4f segundos\n", tempo_gasto_ms / 1000.0f);

    Aluno *d_novoAluno;
    cudaMalloc(&d_novoAluno, sizeof(Aluno));
    cudaMemcpy(d_novoAluno, &novoAluno, sizeof(Aluno), cudaMemcpyHostToDevice);
    predict<<<1, 1>>>(d_kmeans, d_novoAluno);
    cudaDeviceSynchronize();
    cudaMemcpy(&novoAluno, d_novoAluno, sizeof(Aluno), cudaMemcpyDeviceToHost);

    printf("Novo aluno - Media: %.2f, Numero de Faltas: %.2f, Cluster: %d\n",
           novoAluno.media, novoAluno.numeroFaltas, novoAluno.cluster);

    // libera memória
    free(h_kmeans.centroids);
    free(h_alunos);
    cudaFree(d_alunos);
    cudaFree(d_kmeans);
    cudaFree(d_centroids);
    cudaFree(d_novoAluno);

    return 0;
}
