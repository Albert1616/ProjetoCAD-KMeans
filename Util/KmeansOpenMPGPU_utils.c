#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "../Model/aluno.h"
#include "../Model/kmeans.h"

// Utilitários para o KMeans
#pragma omp declare target
float distEuclidiana(Aluno *aluno, Aluno *centroid)
{
    float diffMedia = pow(aluno->media - centroid->media, 2);
    float diffFaltas = pow(aluno->numeroFaltas - centroid->numeroFaltas, 2);

    return sqrt(diffMedia + diffFaltas);
}
#pragma omp end declare target

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
        int index = rand_r(&seed) % model->totalAlunos;
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
    int n = model->totalAlunos;
    int k = model->k;
    Aluno *centroids = model->centroids;

    #pragma omp target teams loop map(tofrom: alunos[0:n]) map(to: centroids[0:k])
    for (int i = 0; i < n; i++)
    {
        int melhorCluster = 0;
        float menorDist = distEuclidiana(&alunos[i], &centroids[0]);

        for (int j = 1; j < k; j++)
        {
            float d = distEuclidiana(&alunos[i], &centroids[j]);
            if (d < menorDist)
            {
                menorDist = d;
                melhorCluster = j;
            }
        }

        alunos[i].cluster = melhorCluster;
    }
}

void updateCentroids(KMeans *model, Aluno *alunos)
{
    int n = model->totalAlunos;
    int k = model->k;
    Aluno *centroids = model->centroids;

    float *sum_media = (float *)calloc(k,sizeof(float));
    float *sum_faltas = (float *)calloc(k, sizeof(float));
    int *count = (int *)calloc(k, sizeof(int));

    #pragma omp target teams distribute parallel for thread_limit(omp_get_max_threads()) map(to: alunos[0:n]) reduction(+:sum_media[:k], sum_faltas[:k], count[:k])
    for (int i = 0; i < n; i++)
    {
        int j = alunos[i].cluster;
        sum_media[j] += alunos[i].media;
        sum_faltas[j] += alunos[i].numeroFaltas;
        count[j]++;
    }

    #pragma omp target teams distribute parallel for thread_limit(omp_get_max_threads()) map(to: sum_media[0:k], sum_faltas[0:k], count[0:k]) map(from: centroids[0:k])
    for (int j = 0; j < k; j++)
    {
        if (count[j] > 0)
        {
            centroids[j].media = sum_media[j] / count[j];
            centroids[j].numeroFaltas = sum_faltas[j] / count[j];
        }
    }

    free(sum_media);
    free(sum_faltas);
    free(count);
}

void fit(KMeans *model, Aluno *alunos)
{
    int n = model->totalAlunos;
    int k = model->k;
    Aluno *centroids = model->centroids;

    double t_start = omp_get_wtime();

    initCentroids(model, alunos);

    Aluno *old_centroids = (Aluno *)malloc(k * sizeof(Aluno));
    
    int convergiu = 0;
    #pragma omp target data map(tofrom: alunos[0:n]) map(tofrom: centroids[0:k], old_centroids[0:k])
    {
        for (int i = 0; i < model->max_iter && !convergiu; i++)
        {
            #pragma omp target teams loop map(to: centroids[0:k]) map(from: old_centroids[0:k])
            for (int j = 0; j < k; j++)
                old_centroids[j] = centroids[j];

            assignClusters(model, alunos);
            updateCentroids(model, alunos);
            
            int nao_convergiu = 0;

            #pragma omp target teams loop reduction(+:nao_convergiu) map(to: centroids[0:k], old_centroids[0:k])
            for (int j = 0; j < k; j++)
            {
                if (distEuclidiana(&centroids[j], &old_centroids[j]) > 0.01f)
                {
                    nao_convergiu++;    
                }
            }

            convergiu = (nao_convergiu == 0);
        }
    }

    free(old_centroids);

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
