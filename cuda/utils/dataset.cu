#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../model/aluno.h"
#include "../model/dataset.h"

int carregarDataset(Aluno *alunos, int numAlunos)
{
    struct timeval start_read, stop_read;

    gettimeofday(&start_read, NULL);

    FILE *file = fopen("../Data/Student_performance_data.csv", "r");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    char linha[4000];
    int index = 0;

    while (fgets(linha, sizeof(linha), file) != NULL && index < numAlunos)
    {
        if (index == 0)
        {
            index++;
            continue;
        }

        char copia[4000];
        strcpy(copia, linha);

        char *colunas[15];
        colunas[0] = strtok(copia, ",");
        for (int i = 1; i < 15; i++)
            colunas[i] = strtok(NULL, ",");

        Aluno aluno;
        aluno.numeroFaltas = atof(colunas[6]);
        aluno.media = atof(colunas[13]);
        aluno.cluster = -1;

        alunos[index - 1] = aluno;
        index++;
    }
    fclose(file);

    gettimeofday(&stop_read, NULL);
    double tempo_gasto = (stop_read.tv_sec - start_read.tv_sec) +
                         (stop_read.tv_usec - start_read.tv_usec) / 1000000.0;

    printf("Tempo gasto apenas lendo o disco (.csv): %.4f segundos\n", tempo_gasto);

    return index - 1;
}

void normalizarAlunos(Aluno *alunos, int total)
{
    float minMedia = alunos[0].media, maxMedia = alunos[0].media;
    float minFaltas = alunos[0].numeroFaltas, maxFaltas = alunos[0].numeroFaltas;

    for (int i = 1; i < total; i++)
    {
        if (alunos[i].media < minMedia)
            minMedia = alunos[i].media;
        if (alunos[i].media > maxMedia)
            maxMedia = alunos[i].media;

        if (alunos[i].numeroFaltas < minFaltas)
            minFaltas = alunos[i].numeroFaltas;
        if (alunos[i].numeroFaltas > maxFaltas)
            maxFaltas = alunos[i].numeroFaltas;
    }

    for (int i = 0; i < total; i++)
    {
        alunos[i].media = (alunos[i].media - minMedia) / (maxMedia - minMedia);
        alunos[i].numeroFaltas = (alunos[i].numeroFaltas - minFaltas) / (maxFaltas - minFaltas);
    }
}
