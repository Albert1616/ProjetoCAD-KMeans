#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "../Model/aluno.h"
#include "../Model/kmeans.h"

// Utilitários para o KMeans
float distEuclidiana(Aluno *aluno, Aluno *centroid)
{
    float diffMedia = pow(aluno->media - centroid->media, 2);
    float diffFaltas = pow(aluno->numeroFaltas - centroid->numeroFaltas, 2);

    return sqrt(diffMedia + diffFaltas);
}

int minListaIndex(float lista[], int tamanho)
{
    float min = lista[0];
    int index = 0;

    for (int i = 0; i < tamanho; i++)
    {
        if (lista[i] < min)
        {
            index = i;
            min = lista[i];
        }
    }

    return index;
}

void initCentroids(KMeans *model, Aluno *alunos)
{
    int listIndex[model->k];
    for (int i = 0; i < model->k; i++)
        listIndex[i] = -1;
    // Serial geração de índices com rand_r() thread-safe.
    // Não paralelizamos essa parte pois requer sincronização complexa para evitar duplicatas.
    unsigned int seed = model->random_state;
    for (int i = 0; i < model->k; i++)
    {
        int index = rand_r(&seed) % alunos->totalAlunos;
        int duplicado = 0;

        for (int j = 0; j < i; j++)
        {
            if (listIndex[j] == index)
            {
                duplicado = 1;
                break;
            }
        }

        if (duplicado)
        {
            i--;
            continue;
        }

        listIndex[i] = index;
    }
    
    for (int i = 0; i < model->k; i++)
    {
        model->centroids[i] = alunos[listIndex[i]];
    }
}

void assignClusters(KMeans *model, Aluno *alunos)
{
    // Paraleliza o loop sobre alunos. Cada iteração usa um array local
    // 'distanciaClusters_local' para evitar condições de corrida.
    #pragma omp target teams loop present(model, alunos)
    for (int i = 0; i < model->totalAlunos; i++)
    {
        float distanciaClusters_local[model->k];

        for (int j = 0; j < model->k; j++)
        {
            distanciaClusters_local[j] = distEuclidiana(&alunos[i], &model->centroids[j]);
        }

        alunos[i].cluster = minListaIndex(distanciaClusters_local, model->k);
    }
}

void updateCentroids(KMeans *model, Aluno *alunos)
{
    #pragma omp target teams loop present(alunos, model)
    for (int j = 0; j < model->k; j++)
    {
        float local_sum_media = 0.0f;
        int local_sum_faltas = 0;
        int local_count = 0;

        for (int i = 0; i < model->totalAlunos; i++)
        {
            if (alunos[i].cluster == j)
            {
                local_sum_media += alunos[i].media;
                local_sum_faltas += alunos[i].numeroFaltas;
                local_count++;
            }
        }

        if (local_count > 0)
        {
            model->centroids[j].media = local_sum_media / local_count;
            model->centroids[j].numeroFaltas = (float)local_sum_faltas / local_count;
        }
    }
}

void fit(KMeans *model, Aluno *alunos)
{
    int n = model->totalAlunos;
    int k = model->k;

    double t_start = omp_get_wtime();

    initCentroids(model, alunos);

    Aluno old_centroids[model->k];
    
    #pragma omp target data map(to: alunos[0:n]) map(tofrom:model->centroids[0:k]) map(to:model[0:1])
    for (int i = 0; i < model->max_iter; i++)
    {
        #pragma omp target teams loop map(from: old_centroids[0:k])
        for (int j = 0; j < model->k; j++)
            old_centroids[j] = model->centroids[j];

        assignClusters(model, alunos);
        updateCentroids(model, alunos);
        int convergiu = 1;

        #pragma omp target teams distribute loop reduction(&&:convergiu)
        for (int j = 0; j < model->k; j++)
        {
            if (distEuclidiana(&model->centroids[j], &old_centroids[j]) > 0.01)
            {
                convergiu = 0;
            }
        }
    }

    double t_end = omp_get_wtime();
    printf("KMeans fit wall time: %.6f s\n", t_end - t_start);
}

void predict(KMeans *model, Aluno *novoAluno)
{
    float distanciaClusters[model->k];

    for (int i = 0; i < model->k; i++)
    {
        distanciaClusters[i] = distEuclidiana(novoAluno, &model->centroids[i]);
    }

    novoAluno->cluster = minListaIndex(distanciaClusters, model->k);
}

float *methodElbow(KMeans *model, Aluno *alunos)
{
    int k_range[] = {2, 3, 4, 5, 6, 7, 8};
    int n = 7;
    float *inertia_values = (float *)malloc(n * sizeof(float));

    // Paraleliza iteração sobre valores de k. Cada thread recebe sua própria cópia
    // do modelo KMeans para evitar race conditions no estado dos centroids.
    for (int i = 0; i < n; i++)
    {
        // Cópia privada do modelo para essa thread
        KMeans model_local = *model;
        model_local.k = k_range[i];
        model_local.centroids = (Aluno *)malloc(model_local.k * sizeof(Aluno));

        // Treina com k local
        fit(&model_local, alunos);

        // Calcula inércia
        float inertia = 0;
        for (int j = 0; j < model_local.totalAlunos; j++)
            inertia += pow(distEuclidiana(&alunos[j], &model_local.centroids[alunos[j].cluster]), 2);

        inertia_values[i] = inertia;
        free(model_local.centroids);
    }
    return inertia_values;
}
