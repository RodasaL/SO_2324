#include "gerebots.h"
#include "gereplayers.h"
#include "utils_motor.h"

GameData DataThreads;

void signal_handler1(int signum, siginfo_t *info, void *secret);
void signal_handler2(int signum, siginfo_t *info, void *secret);

int comandos_flag = 0;

int flagexit = 0;

void *lecommandos(void *arg)
{

    GameData *data = (GameData *)arg;

    char usercmd[100];

    while (data->flagEnd == 0)
    {
        printf("Comando: ");
        // int ch = wgetch(c); // MUITO importante: o input é feito sobre a janela em questão
        fgets(usercmd, sizeof(usercmd), stdin);

        char user[100];
        char cmd[100];

        if (!strcmp(usercmd, "users\n"))
        {
            printf("Jogadores em jogo: \n");
            pthread_mutex_lock(data->ThreadMutex);
            for (int i = 0; i < MAXPLAYERS; i++)
            {
                if (data->allPlayers[i].PID != 0)
                {
                    printf("Nome: %s\tPID: %d\n", data->allPlayers[i].nome, data->allPlayers[i].PID);
                }
            }
            pthread_mutex_unlock(data->ThreadMutex);
        }
        else if (!strcmp(usercmd, "bots\n") && data->flagInscricao == 0)
        {
            for (int i = 0; i < MAXBOTS; i++)
            {
                if (data->bots[i].PID != 0)
                {
                    printf("PID: %d\tDuracao: %d\tCooldown: %d\n", data->bots[i].PID, data->bots[i].duration, data->bots[i].cooldown);
                }
            }
        }
        else if (!strcmp(usercmd, "bmov\n") && data->flagInscricao == 0)
        {
            pthread_mutex_lock(data->ThreadMutex);
            if (data->bloqueiosMoveis >= MAX_BLOQUEIOS_MOVEIS)
            {
                printf("Numero maximo de bloqueios moveis atingido\n");
            }
            else
            {
                data->bloqueiosMoveis++;
            }
            pthread_mutex_unlock(data->ThreadMutex);
            continue;
        }
        else if (!strcmp(usercmd, "rbm\n") && data->flagInscricao == 0)
        {
            pthread_mutex_lock(data->ThreadMutex);
            if (data->bloqueiosMoveis <= 0)
            {
                printf("Nao ha bloqueios moveis para remover\n");
            }
            else
            {
                data->bloqueiosMoveis--;
            }

            pthread_mutex_unlock(data->ThreadMutex);
            continue;
        }
        else if (!strcmp(usercmd, "begin\n"))
        {
            pthread_mutex_lock(data->ThreadMutex);
            if (data->flagInscricao == 0)
                printf("Jogo ja comecou\n");
            else
                data->flagInscricao = 0;
            pthread_mutex_unlock(data->ThreadMutex);
            continue;
        }
        else if (!strcmp(usercmd, "end\n"))
        {
            printf("Fim do jogo\n");

            data->flagInscricao = 0; // Para caso o jogo ainda nao tenha comecado
            data->flagContinua = 0;
            data->flagEnd = 1;
        }
        else if (!strcmp(usercmd, "lab\n") && data->flagInscricao == 0) // Comando adicional para o Admin ver o labirinto
        {
            pthread_mutex_lock(data->ThreadMutex);
            printGrid(data->labirinto);
            pthread_mutex_unlock(data->ThreadMutex);
        }
        else if (sscanf(usercmd, "%s %s", cmd, user) == 2)
        {
            int kickSuccess = 0;
            if (!strcmp(cmd, "kick"))
            {
                pthread_mutex_lock(data->ThreadMutex);
                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    if (data->allPlayers[i].PID != 0)
                    {
                        if (!strcmp(data->allPlayers[i].nome, user))
                        {
                            data->labirinto[data->allPlayers[i].posicao.y][data->allPlayers[i].posicao.x] = ' ';
                            sigqueue(data->allPlayers[i].PID, SIGINT, (const union sigval)0);
                            data->allPlayers[i].PID = 0;
                            strcpy(data->allPlayers[i].nome, " ");
                            pthread_mutex_unlock(data->ThreadMutex);

                            kickSuccess = 1;
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(data->ThreadMutex);
                if(kickSuccess == 1)
                {
                    

                    // enviar msg para todos os jogadores

                    MSG kick_notif;
                    kick_notif.type = 5;
                    sprintf(kick_notif.msg, "O jogador %s foi kickado do jogo", user);

                    int hasPlayers = 0;

                    pthread_mutex_lock(data->ThreadMutex);
                    for (int i = 0; i < MAXPLAYERS; i++)
                    {
                        if (data->allPlayers[i].PID != 0)
                        {
                            hasPlayers = 1;
                            enviaMSG(kick_notif, data->allPlayers[i]);
                        }
                    }

                    if (hasPlayers == 0)
                    {
                        printf("Nao ha jogadores em jogo\n");
                        data->flagContinua = 0;
                        data->flagEnd = 1;
                    }
                    pthread_mutex_unlock(data->ThreadMutex);
                }
                else
                {
                    printf("Jogador nao encontrado\n");
                }
            }
            else
            {
                printf("Comando nao reconhecido\n");
            }
        }
        else
        {

            printf("Comando nao reconhecido\n");
        }
    }
}

int jogo()
{

    // Signals
    struct sigaction sa_int;
    sa_int.sa_sigaction = signal_handler1;
    sa_int.sa_flags = SA_SIGINFO;

    // send player array to signal handler

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        strcpy(DataThreads.allPlayers[i].nome, " ");
        DataThreads.allPlayers[i].PID = 0;
    }

    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = signal_handler2;
    sa_usr1.sa_flags = SA_SIGINFO; // Usado para sair do modo leitura de cmds;

    sigaction(SIGUSR1, &sa_usr1, NULL);

    if (mkfifo(MOTOR_FIFO, 0666) == -1)
    {
        perror("Motor ja aberto\n");
        exit(1);
    }

    // Variaveis de ambiente

    int inscricao = getenv("INSCRICAO") == NULL ? 9999 : atoi(getenv("INSCRICAO"));
    int minplayers = getenv("NPLAYERS") == NULL ? 9999 : atoi(getenv("NPLAYERS"));
    int duracao = getenv("DURACAO") == NULL ? 9999 : atoi(getenv("DURACAO"));
    int decremento = getenv("DECREMENTO") == NULL ? 0 : atoi(getenv("DECREMENTO"));

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_t CommandManager;
    DataThreads.flagEnd = 0;
    DataThreads.flagInscricao = 1;
    DataThreads.ThreadMutex = &mutex;

    pthread_create(&CommandManager, NULL, &lecommandos, &DataThreads);

    printf("Inscricao: %d\n", inscricao);
    printf("Minimo de jogadores: %d\n", minplayers);
    printf("Duracao: %d\n", duracao);
    printf("Decremento: %d\n", decremento);

    recebeJogadores(&DataThreads, MAXPLAYERS, minplayers, inscricao);

    DataThreads.levelDuration = duracao;

    for (int i = 0; i < 3; i++)
    {
        printf("Nivel %d\n", i + 1);
        inicializarNivel(DataThreads.labirinto, i + 1, DataThreads.allPlayers);

        pthread_t PlayerReciever;
        pthread_t RockManager;
        pthread_t BMCleanup;

        DataThreads.levelStartTime = time(NULL);

        DataThreads.flagContinua = 1;
        DataThreads.threadID = 1;
        DataThreads.nivel = i + 1;
        DataThreads.bloqueiosMoveis = 0;

        pthread_create(&PlayerReciever, NULL, &atendeJogador, &DataThreads);

        pthread_create(&RockManager, NULL, &gerePedra, &DataThreads);
        pthread_create(&BMCleanup, NULL, &gereBMeCleanup, &DataThreads);

        while (DataThreads.flagContinua)
        {
            ;
        }

        pthread_kill(RockManager, SIGUSR1);
        pthread_kill(CommandManager, SIGUSR1);

        pthread_join(PlayerReciever, NULL);
        pthread_join(RockManager, NULL);
        pthread_join(BMCleanup, NULL);

        if (DataThreads.flagEnd == 1)
        {
            break;
        }

        if (DataThreads.levelDuration - decremento > 0)
        {
            DataThreads.levelDuration -= decremento;
        }
        else
        {
            DataThreads.levelDuration = 0;
        }
    }

    DataThreads.flagEnd = 1;
    pthread_kill(CommandManager, SIGUSR1);

    pthread_join(CommandManager, NULL);
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (DataThreads.allPlayers[i].PID != 0)
        {
            kill(DataThreads.allPlayers[i].PID, SIGINT);
        }
    }

    pthread_mutex_destroy(&mutex);

    unlink(MOTOR_FIFO);

    return 0;
}

int main()
{
    jogo();

    unlink(MOTOR_FIFO);

    return 0;
}

void signal_handler1(int signum, siginfo_t *info, void *secret)
{

    flagexit = 1;

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (DataThreads.allPlayers[i].PID != 0)
        {
            kill(DataThreads.allPlayers[i].PID, SIGINT);
        }
    }

    unlink(MOTOR_FIFO);

    endwin();
    perror("CTRL + C\n");

    exit(0);
}

void signal_handler2(int signum, siginfo_t *info, void *secret)
{
}