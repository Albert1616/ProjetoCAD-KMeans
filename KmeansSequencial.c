#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Model/aluno.h"
#include "Model/dataset.h"

int main()
{
    // Dados de cada aluno
    Aluno *alunos = (Aluno *)malloc(1000 * sizeof(Aluno));
    carregarDataset(alunos);

    for (int i = 0; i < 100; i++)
    {
        printf("Aluno %d: Media: %.2f, Horas de Estudo: %.2f, Numero de Faltas: %.2f\n", i + 1, alunos[i].media, alunos[i].horasEstudo, alunos[i].numeroFaltas);
    }

    free(alunos);
    return 0;
}