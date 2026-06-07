#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Model/aluno.h"
#include "Model/dataset.h"
#include "Model/Kmeans.h"

#define NUM_CLUSTERS 5
#define MAX_ITERATIONS 400

void exportarResultados(Aluno *alunos, int total)
{
    FILE *file = fopen("resultados.csv", "w");
    fprintf(file, "horasEstudo,numeroFaltas,media,suporte,reforco,cluster\n");

    for (int i = 0; i < total; i++)
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%.2f,%d\n",
                alunos[i].horasEstudo,
                alunos[i].numeroFaltas,
                alunos[i].media,
                alunos[i].suporte,
                alunos[i].reforco,
                alunos[i].cluster);

    fclose(file);
}

int main()
{
    int numeroAlunos = 0;
    // Dados de cada aluno
    Aluno *alunos = (Aluno *)malloc(3000 * sizeof(Aluno));
    numeroAlunos = carregarDataset(alunos);

    normalizarAlunos(alunos, numeroAlunos);

    // // Configuração do modelo
    KMeans kmeans;
    kmeans.k = NUM_CLUSTERS;
    kmeans.max_iter = MAX_ITERATIONS;
    kmeans.random_state = 42;
    kmeans.centroids = (Aluno *)malloc(kmeans.k * sizeof(Aluno));
    kmeans.totalAlunos = numeroAlunos;

    // // Treinamento
    fit(&kmeans, alunos);
    int numAlunosCluster = 0;

    // // Visualização dos resultados
    // for (int i = 0; i < kmeans.k; i++)
    // {
    //     numAlunosCluster = 0;
    //     for (int j = 0; j < kmeans.totalAlunos; j++)
    //     {
    //         if (alunos[j].cluster == i)
    //         {
    //             // printf("Aluno %d: Media: %.2f, Horas de Estudo: %.2f, Numero de Faltas: %.2f\n, Cluster: %d\n", j + 1, alunos[j].media, alunos[j].horasEstudo, alunos[j].numeroFaltas, alunos[j].cluster);
    //             numAlunosCluster++;
    //         }
    //     }
    //     printf("Cluster %d: %d alunos\n", i, numAlunosCluster);
    // }

    // // for (int i = 0; i < 100; i++)
    // // {
    // //     printf("Aluno %d: Media: %.2f, Horas de Estudo: %.2f, Numero de Faltas: %.2f\n", i + 1, alunos[i].media, alunos[i].horasEstudo, alunos[i].numeroFaltas);
    // // }

    exportarResultados(alunos, numeroAlunos);
    float *inertia = methodElbow(&kmeans, alunos);

    for (int i = 0; i < 7; i++)
    {
        printf("k: %d, Inertia: %.4f\n", i + 2, inertia[i]);
    }

    free(kmeans.centroids);
    free(alunos);
    return 0;
}