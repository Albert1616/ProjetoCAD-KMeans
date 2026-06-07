#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Model/aluno.h"
#include "../Model/dataset.h"

char *retornarDadoPorIndice(char *linha, int index)
{
    char copia[strlen(linha) + 1];
    strcpy(copia, linha);
    int contador = 0;

    char *dados = strtok(copia, ",");

    while (dados != NULL)
    {
        if (contador == index)
        {
            return strdup(dados);
        }
        dados = strtok(NULL, ",");
        contador++;
    }

    return NULL;
}

int carregarDataset(Aluno *alunos)
{
    FILE *file = fopen("Data/Student_performance_data.csv", "r");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    char linha[4000];
    int index = 0;

    while (fgets(linha, sizeof(linha), file) != NULL)
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
        aluno.horasEstudo = atof(colunas[5]);
        aluno.numeroFaltas = atof(colunas[6]);
        aluno.reforco = atoi(colunas[7]);
        aluno.suporte = atoi(colunas[8]);
        aluno.media = atof(colunas[13]);
        aluno.cluster = -1;

        alunos[index - 1] = aluno;
        index++;
    }

    fclose(file);
    return index - 1;
}

void normalizarAlunos(Aluno *alunos, int total)
{
    float minMedia = alunos[0].media, maxMedia = alunos[0].media;
    float minHoras = alunos[0].horasEstudo, maxHoras = alunos[0].horasEstudo;
    float minFaltas = alunos[0].numeroFaltas, maxFaltas = alunos[0].numeroFaltas;
    float minSuporte = alunos[0].suporte, maxSuporte = alunos[0].suporte;

    for (int i = 1; i < total; i++)
    {
        if (alunos[i].media < minMedia)
            minMedia = alunos[i].media;
        if (alunos[i].media > maxMedia)
            maxMedia = alunos[i].media;

        if (alunos[i].horasEstudo < minHoras)
            minHoras = alunos[i].horasEstudo;
        if (alunos[i].horasEstudo > maxHoras)
            maxHoras = alunos[i].horasEstudo;

        if (alunos[i].numeroFaltas < minFaltas)
            minFaltas = alunos[i].numeroFaltas;
        if (alunos[i].numeroFaltas > maxFaltas)
            maxFaltas = alunos[i].numeroFaltas;

        if (alunos[i].suporte < minSuporte)
            minSuporte = alunos[i].suporte;
        if (alunos[i].suporte > maxSuporte)
            maxSuporte = alunos[i].suporte;
    }

    for (int i = 0; i < total; i++)
    {
        alunos[i].media = (alunos[i].media - minMedia) / (maxMedia - minMedia);
        alunos[i].horasEstudo = (alunos[i].horasEstudo - minHoras) / (maxHoras - minHoras);
        alunos[i].numeroFaltas = (alunos[i].numeroFaltas - minFaltas) / (maxFaltas - minFaltas);
        alunos[i].suporte = (alunos[i].suporte - minSuporte) / (maxSuporte - minSuporte);
    }
}