#include "gerebots.h"
#include "gereplayers.h"
#include "utils_motor.h"

void * gerePedra(void * args)
{
    GameData * data = (GameData *) args;

    for(int i = 0; i < MAX_PEDRAS; i++)
    {
        data->pedras[i].duration_seconds = 0;
    }

    // create nivel + 1 bots

    int numBots = data->nivel + 1;

    int pipefd[2];

    int init_duracao = 5 * numBots;
    int duration_increment = -5;
    int init_cooldown = 30;
    int cooldown_increment = -5;

    for(int i = 0; i < numBots; i++)
    {
        if(pipe(pipefd) == -1)
        {
            perror("Erro a criar pipe\n");
            exit(1);
        }

        pid_t pid = fork();

        if(pid == -1)
        {
            perror("Erro a criar processo\n");
            exit(1);
        }

        if(pid == 0)
        {
            close(pipefd[0]);
            dup2(pipefd[1], 1);
            close(pipefd[1]);

            execl("./bot", "./bot", itoa(init_cooldown, 10), itoa(init_duracao, 10), NULL);
            perror("Erro a executar bot\n");
            exit(1);
        }
        
        close(pipefd[1]);
    }

    int flag = 0;

    while(!flag)
    {
        
            char buffer[100];
            int nread = read(pipefd[0], buffer, sizeof(buffer));
            buffer[nread] = '\0';

            if(nread > 0)
            {
                int x, y, duracao;
                sscanf(buffer, "%d %d %d", &x, &y, &duracao);
                Posicao pos;
                pos.x = x;
                pos.y = y;
                if(data->labirinto[y][x] != 'b')
                    continue;
                for(int i = 0; i < MAX_PEDRAS; i++)
                {
                    if(data->pedras[i].duration_seconds == 0)
                    {
                        data->pedras[i].posicao = pos;
                        data->pedras[i].duration_seconds = duracao;
                        data->pedras[i].creation_time = time(NULL);
                        
                        data->labirinto[y][x] = 'p';

                        printGrid(data->labirinto);                                                     
                        break;
                    }
                }
            }
        
    }

}
    