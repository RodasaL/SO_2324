#include "jogoui_utils.h"

void printGridJogo(WINDOW *jan, char arr[MAXCOORD_Y][MAXCOORD_X])
{
    wclear(jan);
    for (int i = 0; i < MAXCOORD_Y; i++)
    {
        for (int j = 0; j < MAXCOORD_X; j++)
        {
            if(arr[i][j] == 'b')
                wprintw(jan," ");
            else 
                wprintw(jan,"%c", arr[i][j]);
        }
        wprintw(jan, "\n");
    }
    //refresh();
    wrefresh(jan);
}

void *readFromFIFO(void *args)
{

    TDATA *data = (TDATA *)args;

    int fd_jogador = open(data->FIFO, O_RDONLY);

    while (data->flagEnd == 0)
    {
        MSG_UI MSG_REC;

        int size = read(fd_jogador, &MSG_REC, sizeof(MSG_REC));
        if (size > 0)
        {
            switch (MSG_REC.type)
            {
            case 0:
                if (MSG_REC.connectionSuccess == 1)
                {
                    wprintw(data->janelaCmd, "A espera de jogadores!");
                    wrefresh(data->janelaCmd);
                    data->espectador = 0;
                }
                else if (MSG_REC.connectionSuccess == 0)
                {
                    kill(getpid(), SIGINT);
                }
                else if(MSG_REC.connectionSuccess == 2)
                {
                    wclear(data->janelaCmd);
                    wprintw(data->janelaCmd, "A espera de jogadores! Entrou como espectador!");
                    wrefresh(data->janelaCmd);
                    data->espectador = 1;
                    }
                break;
            case 1:
                printGridJogo(data->janelaJogo, MSG_REC.labirinto);
               
                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    data->allPlayers[i] = MSG_REC.arrJogadores[i];
                }
                
                data->flag = 1;
                break;
            case 4:
                wclear(data->janelaCmd);
                wrefresh(data->janelaJogo);
                wprintw(data->janelaCmd, "Mensagem recebida de %s: [%s]", MSG_REC.player.nome, MSG_REC.msg);
                
                wrefresh(data->janelaCmd);
                break;

            case 5:
                wclear(data->janelaCmd);
                wrefresh(data->janelaJogo);
                wprintw(data->janelaCmd, "[SYSTEM]: [%s]", MSG_REC.msg);
                wrefresh(data->janelaCmd);
                break;

            }
        }
    }

    close(fd_jogador);
}