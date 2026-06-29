#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "Model/aluno.h"
#include "Model/dataset.h"
#include "Model/kmeans.h"

#define NUM_CLUSTERS 3
#define MAX_ITERATIONS 10000

void exportarResultados(Aluno *alunos, int total)
{
    FILE *file = fopen("resultados_openmp.csv", "w");
    fprintf(file, "numeroFaltas,media,cluster\n");

    for (int i = 0; i < total; i++)
        fprintf(file, "%.2f,%.2f,%d\n",
                alunos[i].numeroFaltas,
                alunos[i].media,
                alunos[i].cluster);

    fclose(file);
}

int main(int argc, char **argv)
{
    // Usuário pode definir número de threads em tempo de execução: ./KMeansOpenMP <threads>
    if (argc > 1)
    {
        int nt = atoi(argv[1]);
        if (nt > 0)
            omp_set_num_threads(nt);
    }

    printf("Using %d OpenMP threads\n", omp_get_max_threads());
    // Dados de cada aluno
    int totalLinhas = obterNumeroLinhas("Data/Student_performance_data.csv");
    if (totalLinhas <= 0) totalLinhas = 3000;
    int num_repeticoes = 1;
    if (argc > 2) {
        totalLinhas = atoi(argv[2]);
    }
    if (argc > 3) {
        num_repeticoes = atoi(argv[3]);
    }
    Aluno *alunos = (Aluno *)malloc(totalLinhas * sizeof(Aluno));
    int numeroAlunos = carregarDataset(alunos, totalLinhas);

    normalizarAlunos(alunos, numeroAlunos);

    // // Configuração do modelo
    KMeans kmeans;
    kmeans.k = NUM_CLUSTERS;
    kmeans.max_iter = MAX_ITERATIONS;
    kmeans.random_state = 42;
    kmeans.centroids = (Aluno *)malloc(kmeans.k * sizeof(Aluno));
    kmeans.totalAlunos = numeroAlunos;

    // Treinamento
    double inicio = omp_get_wtime();
    for (int r = 0; r < num_repeticoes; r++) {
        kmeans.random_state = 42 + r;
        fit(&kmeans, alunos);
    }
    double fim = omp_get_wtime();
    printf("Duração do treinamento: %.6f\n", fim - inicio);

    exportarResultados(alunos, numeroAlunos);

    // Visulalizar centroides
    for (int i = 0; i < kmeans.k; i++)
    {
        printf("Centroid %d - Media: %.2f, Numero de Faltas: %.2f\n", i, kmeans.centroids[i].media, kmeans.centroids[i].numeroFaltas);
    }

    // Predição de um novo aluno
    Aluno novoAluno;
    novoAluno.media = 0.95;
    novoAluno.numeroFaltas = 0.95;

    predict(&kmeans, &novoAluno);
    printf("Novo aluno - Media: %.2f, Numero de Faltas: %.2f, Cluster: %d\n", novoAluno.media, novoAluno.numeroFaltas, novoAluno.cluster);

    free(kmeans.centroids);
    free(alunos);

    return 0;
}