#ifndef KMEANS_H
#define KMEANS_H
#include "Aluno.h"

typedef struct
{
    int k;
    int max_iter;
    int random_state;
    int totalAlunos;
    Aluno *centroids;
} KMeans;

void initCentroids(KMeans *model, Aluno *alunos);
void assignClusters(KMeans *model, Aluno *alunos);
void updateCentroids(KMeans *model, Aluno *alunos);
void predict(KMeans *model, Aluno *novoAluno);
void fit(KMeans *model, Aluno *alunos);
float *methodElbow(KMeans *model, Aluno *alunos);

#endif // KMEANS_H