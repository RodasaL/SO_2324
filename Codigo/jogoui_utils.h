#ifndef JOGOUI_UTILS_H
#define JOGOUI_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include <signal.h>
#include <error.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_MESSAGE_LENGTH 1024
#define MAXCOORD_X 40
#define MAXCOORD_Y 16
#define MAXPLAYERS 15
#define MAXACTIVEPLAYERS 5

#define CONNECTION_FIFO "connection_fifo"
#define MOTOR_FIFO "motor_fifo"
#define PLAYER_FIFO "player_fifo_%d"

typedef struct Posicao {
    int x;
    int y;
} Posicao, *PPosicao;


typedef struct Player_UI {
    int PID;
    char nome[255];
    char FIFO[255];

    int ativo;


    char letra;

    Posicao posicao;

} Jogador_UI, *PJogador_UI;

typedef struct MSG_UI {

    /**
     * -1 - Inscrição
     * 0 - Conecao
     * 1 - Update labirinto (Motor -> UI)
     * 2 - Update posicao (Ui -> Motor)
    */
    int type;

    Jogador_UI arrJogadores[MAXPLAYERS];

    Jogador_UI player;

    int connectionSuccess;

    char labirinto[MAXCOORD_Y][MAXCOORD_X];

    // 1- cima
    // 2- baixo
    // 3- esquerda
    // 4- direita
    int tipoMovimento;

    char msg[MAX_MESSAGE_LENGTH];

} MSG_UI, *PMSG_UI;

typedef struct TDATA
{
    char FIFO[255];
    WINDOW *janelaJogo;
    WINDOW *janelaCmd;


    Jogador_UI allPlayers[MAXPLAYERS];

    Jogador_UI jogador;

    int espectador;

    int flagEnd;

    int flag;
} TDATA;


void printGridJogo(WINDOW *j,char arr[MAXCOORD_Y][MAXCOORD_X]);
void * readFromFIFO (void * args);

//struct Player jogador;


#endif