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

void carregarDataset(Aluno *alunos)
{
    FILE *file = fopen("Data/Student_performance_data.csv", "r");

    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    char linha[8000];
    int index = 0;

    while (fgets(linha, sizeof(linha), file) != NULL)
    {
        if (index == 0)
        {
            index++;
            continue;
        }

        Aluno aluno;
        aluno.horasEstudo = atof(retornarDadoPorIndice(linha, 5));
        aluno.numeroFaltas = atof(retornarDadoPorIndice(linha, 6));
        aluno.media = atof(retornarDadoPorIndice(linha, 13));

        alunos[index - 1] = aluno;

        index++;
    }
}