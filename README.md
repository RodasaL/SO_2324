S O _ 2 3 2 4 

# Guide:


## Motor:

- LeComandos (Le Comandos)

- recebeJogadores -> gereplayers.c  ( Basicamente gere a conexao inicial do Jogador, recebe o nome e destingue se vai para espectador ou não)

- inicializarNivel -> utils_motor.c (Coloca os players nas posicoes inicias de forma aleatoria o X o Y é sempre o mesmo, da print ao labirinto e manda por MSG para os Players o mesmo)

- atendeJogador->utils_motor.c (Verifica se o tempo acabou, caso tenha acabado encerra. Atencao há "passagem" de Player para Jogador e da handle dos pedidos dos jogadores atraves dos types da estrutura MSG enviada) 
- [A estrutura mais importante é o gameData como se pode ver nesta funcao]

- enviaStandardMSG (prepara o labirinto)-> É chamado no atendeJogador -> gereplayers.c -> Que por sua vez chama o enviaMSG Que faz o write no Fifo

- gerePedra->utils_motor.c (está no nome)

- gereBMeCleanup->utils_motor.c (está no nome)


## JogoUI: 
- WINDOW *janelaexemplo = newwin(dimensoes da janela); 
- imagina que o terminal da para criar janelas la dentro;
- Isto é bom para por exemplo escrever comandos num lando e dar print noutro com o wprintw(JanelaEmQueDouPrint, "Print")
- initscr();  ativa o ncurses
- start_color(); ativa as cores
- noecho(); É para quando uso setas e não as quero ver/dar print tipo aperto (seta para cima e não para aparecer ^up)
- cbreack(); Remove a necessidade de dar Enter no fim de cada comando 

- MSG_UI sendPlayer
- ThreadRecebe ( funcao readFromFIFO com acesso a dataThread ou estrutura TDATA)
- readFromFIFO ->jogoui_utils.c (le do fifo e da update ao ecra limpando o ecra e atualizando wclear wrefresh)
- types recebidos
- 0 que é a inscricao 
- 1 Foi feito um movimento da print do labririnto atualizado ->printGridJogo -> Clear da Janela print do Labirinto e refresh 
- 4 Msg de Player
- 5 Msg do Motor

## Utils_motor.h:
- Structs:
- Posicao 
- Pedra
- Bot
- Player
- MSG
- GameData

## jogoui_utils.h:
- Structs:
- Posicao
- Player_UI
- Msg_UI
- TDATA


- Basicamente o GameData e o MSG é onde tudo acontece o GameData para ter acesso a "Todas" as informacoes e o MSG para saber que tipo de MSG é que se trata comunicacao, movimento, inscricao etc...
- Atencao que cada movimento dos Players no JogoUI é enviado para o MOTOR para o MOTOR atualizar o Labirinto e devolvê-lo.

## Motor:
- Thread:
- Para LerComandos
- Para Atender os pedidos dos Jogadores
- Pare os bots e as Pedras 
- Para os Bloqueios Moveis 

## JogoUI:
- Thread:
- Ler do FIFO

 
 
