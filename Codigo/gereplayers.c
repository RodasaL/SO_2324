#include "gereplayers.h"

void enviaMSG(MSG msg, Jogador jogador)
{
    int fd = open(jogador.FIFO, O_WRONLY);
    if (fd == -1)
    {
        perror("Erro a abrir FIFO do jogador\n");
        exit(1);
    }

    if (write(fd, &msg, sizeof(MSG)) == -1)
    {
        perror("Erro a escrever no FIFO do jogador\n");
        exit(1);
    }

    close(fd);
}

void enviaStandardMSG(MSG msg, GameData *data) // Funcao que envia msg com parametros "standard"
{

    // Parametros standard:
    // - Labirinto
    // - Array de jogadores

    msg.type = 1;

    for (int i = 0; i < MAXCOORD_Y; i++)
    {
        for (int j = 0; j < MAXCOORD_X; j++)
        {
            msg.labirinto[i][j] = data->labirinto[i][j];
        }
    }

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        msg.arrJogadores[i] = data->allPlayers[i];
    }

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (data->allPlayers[i].PID != 0)
        {
            enviaMSG(msg, data->allPlayers[i]);
        }
    }
}

int recebeJogadores(GameData *data, int maxJogadores, int minplayers, int duracao)
{
    time_t connection_start = time(NULL);

    pthread_mutex_lock(data->ThreadMutex);
    for (int i = 0; i < maxJogadores; i++)
    {
        data->allPlayers[i].PID = 0;
        strcpy(data->allPlayers[i].nome, " ");
    }
    pthread_mutex_unlock(data->ThreadMutex);
    int fd = open(MOTOR_FIFO, O_RDONLY | O_NONBLOCK);

    if (fd == -1)
    {
        perror("Erro a abrir FIFO do motor\n");
        exit(1);
    }

    int jogadoresALigar = 0;

    int flagConexao = 0;

    do
    {
        MSG msg;
        msg.type = 0;

        MSG rec;
        int nread;

        nread = read(fd, &rec, sizeof(MSG));

        if ((time(NULL) - connection_start >= duracao && jogadoresALigar >= minplayers) || data->flagInscricao == 0)
        {
            flagConexao = 1;
            data->flagInscricao = 0; // Caso tenha sido o tempo a acabar em vez de o admin ter dado begin
        }

        if (nread > 0)
        {
            if (rec.type == -1)
            {
                int flagLigado = 0;
                int fd_client = open(rec.player.FIFO, O_WRONLY);

                if(fd_client == -1)
                {
                    perror("Erro a abrir FIFO do jogador\n");
                    exit(1);
                }
                for (int i = 0; i < MAXPLAYERS; i++)
                {

                    if (strcmp(data->allPlayers[i].nome, rec.player.nome) == 0)
                    {
                        msg.connectionSuccess = 0;

                        if(write(fd_client, &msg, sizeof(msg)) == -1)
                        {
                            perror("Erro a escrever no FIFO do jogador\n");
                            exit(1);
                        }
                        break;
                    }
                    if (data->allPlayers[i].PID == 0)
                    {

                        data->allPlayers[i] = rec.player;
                        jogadoresALigar++;
                        flagLigado = 1;

                        // Se total de jogadores for mais do que MAXACTIVEPLAYERS, mete o ativo a 0
                        if (jogadoresALigar > MAXACTIVEPLAYERS)
                        {
                            data->allPlayers[i].ativo = 0;
                            msg.connectionSuccess = 2;
                            printf("Espectador %s com pid %d, fifo %s\n", data->allPlayers[i].nome, data->allPlayers[i].PID, data->allPlayers[i].FIFO);
                        }
                        else
                        {
                            data->allPlayers[i].ativo = 1;
                            msg.connectionSuccess = 1;
                            printf("Jogador %s com pid %d, fifo %s\n", data->allPlayers[i].nome, data->allPlayers[i].PID, data->allPlayers[i].FIFO);
                        }

                        fflush(stdout);

                        if(write(fd_client, &msg, sizeof(msg)) == -1)
                        {
                            perror("Erro a escrever no FIFO do jogador\n");
                            exit(1);
                        }
                        break;
                    }
                }
                if (flagLigado == 0)
                {
                    msg.connectionSuccess = 0;

                    if(write(fd_client, &msg, sizeof(msg)) == -1)
                    {
                        perror("Erro a escrever no FIFO do jogador\n");
                        exit(1);
                    }
                }
                close(fd_client);
            }
        }
    } while (flagConexao == 0);

    close(fd);

    return jogadoresALigar;
}