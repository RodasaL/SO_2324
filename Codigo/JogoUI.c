#include "jogoui_utils.h"

void signal_handler(int signum, siginfo_t *info, void *secret);
void signal_handler2(int signum, siginfo_t *info, void *secret);

void desenhaMapa(WINDOW *janela, int tipo)
{
    // quando temos o scroll ativo, não deveremos ter a borda desenhada na janela para não termos o problema escrever em cima das bordas
    if (tipo == 1)
    {
        scrollok(janela, TRUE); // liga o scroll na "janela".
        wprintw(janela, "\n #> ");
    }
    else
    {
        keypad(janela, TRUE); // para ligar as teclas de direção (aplicar à janela)
        wclear(janela);       // limpa a janela
                              // wborder(janela, '|', '|', '-', '-', '+', '+', '+', '+'); // Desenha uma borda. Nota importante: tudo o que escreverem, devem ter em conta a posição da borda
    }
    refresh();        // necessário para atualizar a janela
    wrefresh(janela); // necessário para atualizar a janela
}

int main(int argc, char *argv[])
{

    // Caso o utilizador não insira um nome
    if (argc != 2)
    {
        fprintf(stderr, "-> %s [Username]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // garante que nome do jogador nao tem caracteres especiais

    for(int i = 0; i < strlen(argv[1]); i++)
    {
        if (argv[1][i] >= 65 && argv[1][i] <= 90 || argv[1][i] >= 97 && argv[1][i] <= 122)
        {
            continue;
        }
        else
        {
            fprintf(stderr, "[ERRO]: Nome do jogador nao pode conter caracteres especiais\n");
            exit(EXIT_FAILURE);
        }
    }

    struct sigaction sa_int;
    sa_int.sa_sigaction = signal_handler;
    sa_int.sa_flags = SA_SIGINFO; 
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = signal_handler2;
    sa_usr1.sa_flags = SA_SIGINFO; // Usado para sair do modo leitura de cmds;

    sigaction(SIGUSR1, &sa_usr1, NULL); 

    int fd_motor = open(MOTOR_FIFO, O_WRONLY);

    if (fd_motor == -1)
    {
        perror("Erro ao abrir fifo de motor");
        exit(0);
    }

    char fifoName[255];
    sprintf(fifoName, PLAYER_FIFO, getpid());
    mkfifo(fifoName, 0666);

    initscr(); // ativa ncurses
    start_color();
    noecho(); // Vou desativar o echo para as setas
    cbreak(); // Desliga a necessidade de enter para enviar comando
    WINDOW *janelaJogo = newwin(MAXCOORD_Y + 3, 255, 1, 1);
    WINDOW *janelaCommandos = newwin(40, 255, MAXCOORD_Y + 4, 1);

    desenhaMapa(janelaJogo, 0);
    desenhaMapa(janelaCommandos, 1);

    Jogador_UI jogador;
    jogador.PID = getpid();
    strcpy(jogador.nome, argv[1]);
    strcpy(jogador.FIFO, fifoName);
    jogador.letra = toupper(jogador.nome[0]);

    MSG_UI sendPlayer;

    sendPlayer.type = -1;
    sendPlayer.player = jogador;

    pthread_t threadRecebe;

    TDATA dataThread;

    dataThread.flag = 0;
    dataThread.janelaCmd = janelaCommandos;
    strcpy(dataThread.FIFO, fifoName);
    dataThread.janelaJogo = janelaJogo;
    dataThread.jogador = jogador;

    pthread_create(&threadRecebe, NULL, &readFromFIFO, &dataThread);

    write(fd_motor, &sendPlayer, sizeof(sendPlayer));

    // Inicializa as cores
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    while (dataThread.flag == 0)
        ;
    // Atualiza a janela para mostrar a borda
    if(dataThread.espectador == 0)
        {
            wclear(janelaCommandos);
            wprintw(janelaCommandos, "Bem vindo %s! a tua letra é %c\n", jogador.nome, jogador.letra);
        }
    else
    {
        wclear(janelaCommandos);
        wprintw(janelaCommandos, "Bem vindo %s! Entrou como espectador!\nNao podera jogar, mas pode enviar mensagens a outros", jogador.nome);
    }
    wrefresh(janelaCommandos);

    char message[MAX_MESSAGE_LENGTH];
    char comando[MAX_MESSAGE_LENGTH];
    char comando2[MAX_MESSAGE_LENGTH];
    char nomeV[MAX_MESSAGE_LENGTH];

    char labirinto[MAXCOORD_Y][MAXCOORD_X];

    MSG_UI sender;

    dataThread.flagEnd = 0;

    int sair = 0;
    const char *p = NULL;
    while (dataThread.flagEnd == 0)
    {
        keypad(janelaJogo, TRUE);
        noecho(); // Vou desativar o echo para as setas
        cbreak(); // Desliga a necessidade de enter para enviar comando
        wrefresh(janelaCommandos);
        wrefresh(janelaJogo);

        int ch = wgetch(janelaJogo);
        switch (ch)
        {
        case KEY_UP:
            sender.type = 2;
            sender.tipoMovimento = 1;
            sender.player = jogador;
            write(fd_motor, &sender, sizeof(sender));
            break;
        case KEY_DOWN:
            sender.type = 2;
            sender.tipoMovimento = 2;
            sender.player = jogador;
            write(fd_motor, &sender, sizeof(sender));
            break;
        case KEY_LEFT:
            sender.type = 2;
            sender.tipoMovimento = 3;
            sender.player = jogador;
            write(fd_motor, &sender, sizeof(sender));
            break;
        case KEY_RIGHT:
            sender.type = 2;
            sender.tipoMovimento = 4;
            sender.player = jogador;
            write(fd_motor, &sender, sizeof(sender));
            break;
        case ' ':
            echo();     // liga echo para ver o input
            nocbreak(); // input + enter
            wclear(janelaCommandos);
            wrefresh(janelaCommandos);
            wprintw(janelaCommandos, "comando ->");
            wscanw(janelaCommandos, " %30[^\n]", comando);
            if (strcmp(comando, "exit") == 0)
            {
                sender.type = 3;
                sender.player = jogador;
                write(fd_motor, &sender, sizeof(sender));

                dataThread.flagEnd = 1;
            }

            if (strcmp(comando, "players") == 0)
            {
                wclear(janelaCommandos);
                wrefresh(janelaCommandos);
                wmove(janelaCommandos, 2, 5);

                for (int i = 0; i < MAXPLAYERS; i++)
                {
                    if (dataThread.allPlayers[i].PID != 0)
                    {
                        wprintw(janelaCommandos, "Nome: %s\t", dataThread.allPlayers[i].nome);
                        wprintw(janelaCommandos, "PID: %d\n", dataThread.allPlayers[i].PID);
                    }
                }

                wrefresh(janelaCommandos);

                break;
            }
            else if (sscanf(comando, "%3s %s %100[^\n]", comando2, nomeV, message) == 3)
            {
                if (strcmp(comando2, "msg") == 0)
                {
                    wclear(janelaCommandos);
                    wrefresh(janelaCommandos);
                    wprintw(janelaCommandos, "Mensagem enviada para %s\n", nomeV);
                    wprintw(janelaCommandos, "Mensagem: %s\n", message);

                    for (int i = 0; i < MAXPLAYERS; i++)
                    {
                        if (dataThread.allPlayers[i].PID != 0)
                        {
                            if (strcmp(dataThread.allPlayers[i].nome, nomeV) == 0)
                            {
                                sender.player = jogador;
                                sender.type = 4;
                                strcpy(sender.msg, message);
                                int fd_rec = open(dataThread.allPlayers[i].FIFO, O_WRONLY);
                                write(fd_rec, &sender, sizeof(sender));
                                close(fd_rec);
                                break;
                            }
                        }
                    }
                    break;
                }
                else
                {
                    printf("Comando não reconhecido!\n");
                    break;
                }
            }

            else
            {
                wclear(janelaCommandos);
                wrefresh(janelaCommandos);
                wmove(janelaCommandos, 2, 5);
                p = "Comando não reconhecido!\n";
                wprintw(janelaCommandos, "- %s\n", p);
                break;
            }
        }
    }

    pthread_kill(threadRecebe, SIGUSR1);
    pthread_join(threadRecebe, NULL);

    close(fd_motor);
    unlink(fifoName);

    endwin(); // Desativa ncurses para a consola ficar a trabalhar
    return 0;

}

void signal_handler(int signum, siginfo_t *info, void *secret)
{
    if (signum == SIGINT)
    {
        char fifoName[255];
        int fd;

        sprintf(fifoName, PLAYER_FIFO, getpid());

        unlink(fifoName);

        endwin(); // Desativa ncurses para a consola ficar a trabalhar
        perror("Adeus [Ctrl + C]");

        exit(0);
    }
}

void signal_handler2(int signum, siginfo_t *info, void *secret)
{
    
}