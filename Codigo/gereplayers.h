#ifndef GERAJOGADOR_H
#define GERAJOGADOR_H


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils_motor.h"



void enviaMSG(MSG msg, Jogador jogador);
void enviaStandardMSG(MSG msg, GameData *data);

int recebeJogadores(GameData *data, int maxJogadores, int minplayers, int duracao);

#endif