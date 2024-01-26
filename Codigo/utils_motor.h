#ifndef UTILS_MOTOR_H
#define UTILS_MOTOR_H

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


#define MAXCOORD_X 40
#define MAXCOORD_Y 16
#define MAXPLAYERS 15
#define MAXACTIVEPLAYERS 5
#define MAXBOTS 10
#define MAX_PEDRAS 50
#define MAX_BLOQUEIOS_MOVEIS 5

#define MAX_MESSAGE_LENGTH 1024
#define CONNECTION_FIFO "connection_fifo"
#define MOTOR_FIFO "motor_fifo"
#define PLAYER_FIFO "player_fifo_%d"
#define FILENAME "nivel%d.txt"

#define MSG_JOGADOR_SAIU "Jogador %s saiu do jogo\n"
#define MSG_JOGADOR_ENTROU "Jogador %s entrou no jogo\n"
#define MSG_ESPECTADOR_ENTROU "Espectador %s entrou no jogo\n"

typedef struct Posicao {
    int x;
    int y;
} Posicao, *PPosicao;

typedef struct Pedra {
    Posicao posicao;
    int duration_seconds;


    // Para bloqueios moveis
    int inUse; // 0 - nao esta a ser usado, 1 - esta a ser usado

    // tempo em que a pedra foi criada
    time_t creation_time; // time_t sege UNIX.


} Pedra;

typedef struct Bot {
    int PID;
    int cooldown;
    int duration;
} Bot;


typedef struct Player {
    int PID;
    char nome[255];
    char FIFO[255];

    int ativo;

    char letra;

    Posicao posicao;

} Jogador, *PJogador;



typedef struct MSG {
    /**
     * -1 - Inscrição
     * 0 - Conecao
     * 1 - Update labirinto (Motor -> UI)
     * 2 - Update posicao (Ui -> Motor)
     * 3 - Saida (UI -> Motor)
     * 4 - MSG (UI -> UI)
     * 5 - MSG (Motor -> UI) // Cenas como jogador entrou, saiu, etc
    */


    int type;

    Jogador arrJogadores[MAXPLAYERS];


    Jogador player;

    int connectionSuccess;

    char labirinto[MAXCOORD_Y][MAXCOORD_X];

    // 1- cima
    // 2- baixo
    // 3- esquerda
    // 4- direita
    int tipoMovimento;

    char msg[MAX_MESSAGE_LENGTH];

} MSG, *PMSG;



#define INIT_COOLDOWN 5
#define INIT_DURATION 5
typedef struct GameData
{
    Jogador allPlayers[MAXPLAYERS];
    Bot bots[MAXBOTS]; 

    //Jogador *jogador; // Jogador que a thread vai atender
    int threadID;
    char labirinto[MAXCOORD_Y][MAXCOORD_X];
    Pedra pedras[MAX_PEDRAS];


    time_t levelStartTime;
    int levelDuration;


    int nPedra;

    int flagContinua;
    int flagEnd;
    int flagInscricao;
    
    int nivel;
    int bloqueiosMoveis;

    pthread_mutex_t * ThreadMutex;


}GameData, *PGameData;





void txtToGrid(char *filename, char labirinto[MAXCOORD_Y][MAXCOORD_X]);
void printGrid(char arr[MAXCOORD_Y][MAXCOORD_X]);
void inicializarNivel(char labirinto[MAXCOORD_Y][MAXCOORD_X], int nivel, Jogador arrJogadores[MAXPLAYERS]);
void * atendeJogador(void *arg);
void * gerePedra(void *arg);
void * gereBMeCleanup(void * args);


#endif