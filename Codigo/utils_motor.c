#include "utils_motor.h"
#include "gereplayers.h"

int copiaLabirinto(char labirinto[MAXCOORD_Y][MAXCOORD_X], char labirintoCopia[MAXCOORD_Y][MAXCOORD_X])
{
    for (int i = 0; i < MAXCOORD_Y; i++)
    {
        for (int j = 0; j < MAXCOORD_X; j++)
        {
            labirintoCopia[i][j] = labirinto[i][j];
        }
    }
}

void txtToGrid(char *filename, char labirinto[MAXCOORD_Y][MAXCOORD_X])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Erro a abrir ficheiro\n");
        exit(1);
    }

    char buffer[MAXCOORD_X + 2]; // +2 to account for '\n' and '\0'

    for (int row = 0; row < MAXCOORD_Y && fgets(buffer, sizeof(buffer), file) != NULL; ++row)
    {
        int col = 0;
        for (int i = 0; buffer[i] != '\0' && buffer[i] != '\n' && col < MAXCOORD_X; ++i)
        {

            labirinto[row][col] = buffer[i];
            col++;
        }
    }

    fclose(file);
}

void printGrid(char arr[MAXCOORD_Y][MAXCOORD_X])
{

    for (int i = 0; i < MAXCOORD_Y; i++)
    {
        for (int j = 0; j < MAXCOORD_X; j++)
        {
            if (arr[i][j] == 'b')
                printf(" ");
            else
                printf("%c", arr[i][j]);
        }
        printf("\n");
    }

    fflush(stdout);
}

void *atendeJogador(void *arg)
{
    PGameData gameData = (GameData *)arg;

    int fd = open(MOTOR_FIFO, O_RDONLY | O_NONBLOCK);

    if (fd == -1)
    {
        perror("Erro a abrir FIFO do jogador\n");
        exit(1);
    }

    MSG msg;

    while (gameData->flagContinua)
    {
        Jogador jogador;
        int nread = read(fd, &msg, sizeof(MSG));
        int i;

        // verifica tempo de jogo

        if (time(NULL) - gameData->levelStartTime >= gameData->levelDuration)
        {
            gameData->flagContinua = 0;
            msg.type = 5;
            sprintf(msg.msg, "Tempo de Jogo Terminou");
            for (int i = 0; i < MAXPLAYERS; i++)
            {
                if (gameData->allPlayers[i].PID != 0)
                {
                    int fd = open(gameData->allPlayers[i].FIFO, O_WRONLY);
                    write(fd, &msg, sizeof(msg));
                    close(fd);
                }
            }
        }

        if (nread > 0)
        {
            for (i = 0; i < MAXPLAYERS; i++)
            {
                pthread_mutex_lock(gameData->ThreadMutex);
                if (gameData->allPlayers[i].PID == msg.player.PID)
                {
                    jogador = gameData->allPlayers[i];
                    pthread_mutex_unlock(gameData->ThreadMutex);
                    break;
                }
                pthread_mutex_unlock(gameData->ThreadMutex);
            }
            switch (msg.type)
            {
            case -1:
                int flag = 0;
                for (i = 0; i < MAXPLAYERS; i++)
                {
                    pthread_mutex_lock(gameData->ThreadMutex);
                    if (gameData->allPlayers[i].PID == 0)
                    {
                        gameData->allPlayers[i] = msg.player;
                        jogador = msg.player;
                        gameData->allPlayers[i].ativo = 0;
                        flag = 1;

                        msg.type = 0;
                        msg.connectionSuccess = 2;

                        int fd = open(msg.player.FIFO, O_WRONLY);
                        write(fd, &msg, sizeof(msg));
                        close(fd);
                        pthread_mutex_unlock(gameData->ThreadMutex);
                        break;
                    }
                    pthread_mutex_unlock(gameData->ThreadMutex);
                }

                if (flag == 0)
                {
                    msg.type = 0;
                    msg.connectionSuccess = 0;

                    int fd = open(msg.player.FIFO, O_WRONLY);
                    write(fd, &msg, sizeof(msg));
                    close(fd);
                }

                MSG env;

                pthread_mutex_lock(gameData->ThreadMutex);
                enviaStandardMSG(env, gameData);
                pthread_mutex_unlock(gameData->ThreadMutex);

                env.type = 5;
                sprintf(env.msg, MSG_ESPECTADOR_ENTROU, jogador.nome);

                pthread_mutex_lock(gameData->ThreadMutex);

                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    if (gameData->allPlayers[i].PID != 0)
                    {
                        int fd = open(gameData->allPlayers[i].FIFO, O_WRONLY);
                        write(fd, &env, sizeof(env));
                        close(fd);
                    }
                }
                pthread_mutex_unlock(gameData->ThreadMutex);

                break;
            case 2: // Movimento

                if (jogador.ativo == 0)
                    break;
                switch (msg.tipoMovimento)
                {
                case 1: // cima
                    pthread_mutex_lock(gameData->ThreadMutex);
                    if (jogador.posicao.y - 1 < 0)
                    {
                        pthread_mutex_unlock(gameData->ThreadMutex);
                        break;
                    }

                    if (gameData->labirinto[jogador.posicao.y - 1][jogador.posicao.x] == 'b')
                    {
                        jogador.posicao.y--;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x] = jogador.letra;
                        gameData->labirinto[jogador.posicao.y + 1][jogador.posicao.x] = 'b';

                        gameData->allPlayers[i] = jogador;
                    }
                    pthread_mutex_unlock(gameData->ThreadMutex);
                    break;
                case 2: // baixo
                    pthread_mutex_lock(gameData->ThreadMutex);
                    if (jogador.posicao.y + 1 >= MAXCOORD_Y)
                    {
                        pthread_mutex_unlock(gameData->ThreadMutex);
                        break;
                    }
                    if (gameData->labirinto[jogador.posicao.y + 1][jogador.posicao.x] == 'b')
                    {
                        jogador.posicao.y++;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x] = jogador.letra;
                        gameData->labirinto[jogador.posicao.y - 1][jogador.posicao.x] = 'b';

                        gameData->allPlayers[i] = jogador;
                    }
                    pthread_mutex_unlock(gameData->ThreadMutex);

                    break;
                case 3: // esquerda
                    pthread_mutex_lock(gameData->ThreadMutex);
                    if (jogador.posicao.x - 1 < 0)
                    {
                        pthread_mutex_unlock(gameData->ThreadMutex);
                        break;
                    }
                    if (gameData->labirinto[jogador.posicao.y][jogador.posicao.x - 1] == 'b')
                    {
                        jogador.posicao.x--;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x] = jogador.letra;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x + 1] = 'b';

                        gameData->allPlayers[i] = jogador;
                    }
                    pthread_mutex_unlock(gameData->ThreadMutex);

                    break;

                case 4: // direita
                    pthread_mutex_lock(gameData->ThreadMutex);
                    if (jogador.posicao.x + 1 >= MAXCOORD_X)
                    {
                        pthread_mutex_unlock(gameData->ThreadMutex);
                        break;
                    }
                    if (gameData->labirinto[jogador.posicao.y][jogador.posicao.x + 1] == 'b')
                    {

                        jogador.posicao.x++;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x] = jogador.letra;
                        gameData->labirinto[jogador.posicao.y][jogador.posicao.x - 1] = 'b';
                        gameData->allPlayers[i] = jogador;
                    }
                    pthread_mutex_unlock(gameData->ThreadMutex);
                    break;
                }

                MSG envia;

                if (jogador.posicao.y == 0)
                {
                    gameData->flagContinua = 0;
                    printf("Nivel %d completo\n", gameData->nivel);
                }

                envia.type = 1;

                pthread_mutex_lock(gameData->ThreadMutex); // Lock para copiar o labirinto
                enviaStandardMSG(envia, gameData);
                pthread_mutex_unlock(gameData->ThreadMutex);

                break;
            case 3:
                printf("Jogador %s saiu\n", jogador.nome);
                fflush(stdout);
                int flagHasPlayer = 0;
                pthread_mutex_lock(gameData->ThreadMutex);

                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    if (gameData->allPlayers[i].PID == jogador.PID)
                    {
                        gameData->allPlayers[i].PID = 0;
                        strcpy(gameData->allPlayers[i].nome, " ");
                        gameData->labirinto[gameData->allPlayers[i].posicao.y][gameData->allPlayers[i].posicao.x] = 'b';
                        continue;
                    }
                    else if (gameData->allPlayers[i].PID != 0)
                    {
                        flagHasPlayer = 1;
                    }
                }

                if (flagHasPlayer == 0)
                {
                    printf("Nao ha jogadores em jogo\n");
                    gameData->flagContinua = 0;
                    gameData->flagEnd = 1;
                }
                else
                
                enviaStandardMSG(envia, gameData);
                pthread_mutex_unlock(gameData->ThreadMutex);

                MSG exit;

                exit.type = 5;
                sprintf(exit.msg, MSG_JOGADOR_SAIU, jogador.nome);

                pthread_mutex_lock(gameData->ThreadMutex);

                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    if (gameData->allPlayers[i].PID != 0)
                    {
                        int fd = open(gameData->allPlayers[i].FIFO, O_WRONLY);
                        write(fd, &exit, sizeof(exit));
                        close(fd);
                    }
                }
                pthread_mutex_unlock(gameData->ThreadMutex);
                break;
            case 4:
                break;
            }
        }
    }
}

void inicializarNivel(char labirinto[MAXCOORD_Y][MAXCOORD_X], int nivel, Jogador arrJogadores[MAXPLAYERS])
{
    char filename[255];

    sprintf(filename, "nivel%d.txt", nivel);

    txtToGrid(filename, labirinto);

    // inicializa random

    srand(time(NULL));

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (arrJogadores[i].PID != 0)
        {
            if(arrJogadores[i].ativo == 1)
            {
                int x, y;
                y = 15;
                do
                {
                    // random from arrayNivelPos
                    x = rand() % MAXCOORD_X;

                } while (labirinto[y][x] != 'b');

                arrJogadores[i].posicao.x = x;
                arrJogadores[i].posicao.y = y;
                labirinto[y][x] = arrJogadores[i].letra;
            }
        }
    }

    printGrid(labirinto);
    MSG send;
    send.type = 1;

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        send.arrJogadores[i] = arrJogadores[i];
    }

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (arrJogadores[i].PID != 0)
        {
            copiaLabirinto(labirinto, send.labirinto);
            int fd;
            if((fd = open(arrJogadores[i].FIFO, O_WRONLY)) == -1)
            {
                perror("Erro a abrir FIFO do jogador\n");
                exit(1);
            }
            if(write(fd, &send, sizeof(send)) == -1)
            {
                perror("Erro a escrever no FIFO do jogador\n");
                exit(1);
            }
            close(fd);
        }
    }
}

void *gerePedra(void *args)
{
    GameData *data = (GameData *)args;

    pthread_mutex_lock(data->ThreadMutex);
    for (int i = 0; i < MAX_PEDRAS; i++)
    {
        data->pedras[i].inUse = 0;
    }
    pthread_mutex_unlock(data->ThreadMutex);

    // create nivel + 1 bots

    int numBots = data->nivel + 1;

    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("Erro a criar pipe\n");
        exit(1);
    }

    int init_duracao = 5 * numBots;
    int duration_increment = -5;
    int init_cooldown = 30;
    int cooldown_increment = -5;

    for (int i = 0; i < numBots; i++)
    {

        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Erro a criar processo\n");
            exit(1);
        }

        if (pid == 0)
        {
            close(1);
            dup(pipefd[1]);
            close(pipefd[0]);
            close(pipefd[1]);

            char buffer[100];
            char buffer2[100];

            sprintf(buffer, "%d", init_cooldown);
            sprintf(buffer2, "%d", init_duracao);
            execl("./bot", "./bot", buffer, buffer2, NULL);
            perror("Erro a executar bot\n");
            exit(1);
        }

        data->bots[i].PID = pid;
        data->bots[i].cooldown = init_cooldown;
        data->bots[i].duration = init_duracao;

        init_cooldown += cooldown_increment;
        init_duracao += duration_increment;

        sleep(1); // para nao criar todos ao mesmo tempo
    }

    close(pipefd[1]);

    int flag = 0;

    while (data->flagContinua)
    {

        char buffer[100];
        int nread = read(pipefd[0], buffer, sizeof(buffer));
        buffer[nread] = '\0';

        if (data->flagContinua == 0)
            break;

        if (nread > 0)
        {
            int x, y, duracao;
            if (sscanf(buffer, "%d %d %d", &x, &y, &duracao) != 3)
            {
                perror("Erro a ler pipe\n");
                exit(1);
            }

            Posicao pos;
            pos.x = x;
            pos.y = y;
            if (data->labirinto[y][x] != 'b')
                continue;
            for (int i = 0; i < MAX_PEDRAS; i++)
            {
                pthread_mutex_lock(data->ThreadMutex);
                if (data->pedras[i].inUse == 0)
                {
                    data->pedras[i].inUse = 1;
                    data->pedras[i].posicao = pos;
                    data->pedras[i].duration_seconds = duracao;
                    data->pedras[i].creation_time = time(NULL);

                    data->labirinto[y][x] = 'p';

                    for (int i = 0; i < MAXPLAYERS; i++)
                    {
                        if (data->allPlayers[i].PID != 0)
                        {
                            MSG send;
                            send.type = 1;
                            send.player = data->allPlayers[i];
                            copiaLabirinto(data->labirinto, send.labirinto);
                            int fd;
                            if((fd = open(data->allPlayers[i].FIFO, O_WRONLY)) == -1)
                            {
                                perror("Erro a abrir FIFO do jogador\n");
                                exit(1);
                            }
                            if(write(fd, &send, sizeof(send)) == -1)
                            {
                                perror("Erro a escrever no FIFO do jogador\n");
                                exit(1);
                            }
                            close(fd);
                        }
                    }

                    pthread_mutex_unlock(data->ThreadMutex);
                    break;
                }
                pthread_mutex_unlock(data->ThreadMutex);
            }
        }
    }

    int p = 0;
    for (int i = 0; i < numBots; i++)
    {
        p = data->bots[i].PID;
        sigqueue(p, SIGINT, (union sigval)0);
        wait(&p);
    }

    close(pipefd[0]);
}

void *gereBMeCleanup(void *args)
{
    GameData *data = (GameData *)args;

    // inicializa random

    srand(time(NULL));

    // array de pedras para guardar bloqueios moveis;

    int totalBloqueiosMoveis = 0;

    Pedra bloqueiosMoveis[MAX_BLOQUEIOS_MOVEIS];

    for (int i = 0; i < MAX_BLOQUEIOS_MOVEIS; i++)
    {
        bloqueiosMoveis[i].inUse = 0;
    }

    while (data->flagContinua)
    {
        for (int i = 0; i < MAX_PEDRAS; i++)
        {
            pthread_mutex_lock(data->ThreadMutex);
            if (data->pedras[i].inUse != 0)
            {
                if (time(NULL) - data->pedras[i].creation_time >= data->pedras[i].duration_seconds)
                {
                    data->labirinto[data->pedras[i].posicao.y][data->pedras[i].posicao.x] = 'b';
                    data->pedras[i].duration_seconds = 0;
                    data->pedras[i].inUse = 0;
                    // printGrid(data->labirinto);
                }
            }
            pthread_mutex_unlock(data->ThreadMutex);
        }

        if (data->bloqueiosMoveis > totalBloqueiosMoveis && totalBloqueiosMoveis < MAX_BLOQUEIOS_MOVEIS)
        {
            for (int i = 0; i < MAX_BLOQUEIOS_MOVEIS; i++)
            {
                pthread_mutex_lock(data->ThreadMutex);
                if (bloqueiosMoveis[i].inUse == 0)
                {
                    bloqueiosMoveis[i].inUse = 1;
                    bloqueiosMoveis[i].duration_seconds = 0;
                    bloqueiosMoveis[i].creation_time = time(NULL);
                    do
                    {
                        bloqueiosMoveis[i].posicao.x = rand() % MAXCOORD_X;
                        bloqueiosMoveis[i].posicao.y = rand() % MAXCOORD_Y;
                    } while (data->labirinto[bloqueiosMoveis[i].posicao.y][bloqueiosMoveis[i].posicao.x] != 'b');

                    data->labirinto[bloqueiosMoveis[i].posicao.y][bloqueiosMoveis[i].posicao.x] = 'B';
                    totalBloqueiosMoveis++;
                    pthread_mutex_unlock(data->ThreadMutex);
                    break;
                }
                pthread_mutex_unlock(data->ThreadMutex);
            }
        }
        else if (data->bloqueiosMoveis < totalBloqueiosMoveis && totalBloqueiosMoveis > 0)
        {
            for (int i = MAX_BLOQUEIOS_MOVEIS - 1; i >= 0; i--)
            {
                pthread_mutex_lock(data->ThreadMutex);
                if (bloqueiosMoveis[i].inUse == 1)
                {
                    bloqueiosMoveis[i].inUse = 0;
                    data->labirinto[bloqueiosMoveis[i].posicao.y][bloqueiosMoveis[i].posicao.x] = 'b';
                    totalBloqueiosMoveis--;
                    pthread_mutex_unlock(data->ThreadMutex);
                    break;
                }
                pthread_mutex_unlock(data->ThreadMutex);
            }
        }

        // Move bloqueios moveis

        for (int i = 0; i < MAX_PEDRAS; i++)
        {
            pthread_mutex_lock(data->ThreadMutex);
            if (bloqueiosMoveis[i].inUse == 1)
            {
                if (time(NULL) - bloqueiosMoveis[i].creation_time >= 1)
                {
                    int x = bloqueiosMoveis[i].posicao.x;
                    int y = bloqueiosMoveis[i].posicao.y;
                    int x2, y2;
                    do
                    {
                        // Ok aqui vamos movelo uma casa para cima, baixo, esquerda ou direita
                        int movimento = rand() % 4;

                        x2 = x;
                        y2 = y;

                        switch (movimento)
                        {
                        case 0: // cima
                            y2--;
                            break;
                        case 1: // baixo
                            y2++;
                            break;
                        case 2: // esquerda
                            x2--;
                            break;
                        case 3: // direita
                            x2++;
                            break;
                        }

                        if(x2 < 0 || x2 >= MAXCOORD_X || y2 < 0 || y2 >= MAXCOORD_Y)
                            continue;
                    } while (data->labirinto[y2][x2] != 'b');

                    data->labirinto[y][x] = 'b';
                    data->labirinto[y2][x2] = 'B';
                    bloqueiosMoveis[i].posicao.x = x2;
                    bloqueiosMoveis[i].posicao.y = y2;
                }
            }
            pthread_mutex_unlock(data->ThreadMutex);
        }

        // enviar labirinto para todos os jogadores

        MSG send;

        pthread_mutex_lock(data->ThreadMutex);
        enviaStandardMSG(send, data);
        pthread_mutex_unlock(data->ThreadMutex);

        sleep(1);
    }
}