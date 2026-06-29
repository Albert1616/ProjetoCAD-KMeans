#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "Model/aluno.h"
#include "Model/dataset.h"
#include "Model/kmeans.h"

#define NUM_CLUSTERS 3
#define MAX_ITERATIONS 10000
#define NUM_ALUNOS 1000000
#define NUM_FIT_ITERATIONS 50

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
    // Dados de cada aluno
    Aluno *alunos = (Aluno *)malloc(NUM_ALUNOS * sizeof(Aluno));
    int numeroAlunos = carregarDataset(alunos, NUM_ALUNOS);

    normalizarAlunos(alunos, numeroAlunos);

    // // Configuração do modelo
    KMeans kmeans;
    kmeans.k = NUM_CLUSTERS;
    kmeans.max_iter = MAX_ITERATIONS;
    kmeans.random_state = 42;
    kmeans.centroids = (Aluno *)malloc(kmeans.k * sizeof(Aluno));
    kmeans.totalAlunos = numeroAlunos;

    double inicio = omp_get_wtime();
    // Treinamento
    fit(&kmeans, alunos, NUM_FIT_ITERATIONS);
    double fim = omp_get_wtime();

    printf("Duração do treinamento: %.2f\n", fim - inicio);

    exportarResultados(alunos, numeroAlunos);

    // Visulalizar centroides
    for (int i = 0; i < kmeans.k; i++)
    {
        printf("Centroid %d - Media: %.2f, Numero de Faltas: %.2f\n", i, kmeans.centroids[i].media, kmeans.centroids[i].numeroFaltas);
    }

    // Predição de um novo aluno
    Aluno novoAluno;
    novoAluno.media = 0.72;
    novoAluno.numeroFaltas = 0.30;

    predict(&kmeans, &novoAluno);
    printf("Novo aluno - Media: %.2f, Numero de Faltas: %.2f, Cluster: %d\n", novoAluno.media, novoAluno.numeroFaltas, novoAluno.cluster);

    free(kmeans.centroids);
    free(alunos);

    return 0;
}