all: motor bot jogoui

bot: bot.c
	gcc -o bot bot.c
	
motor: motor.c utils_motor.c gereplayers.c
	gcc -o motor motor.c utils_motor.c gereplayers.c -lncurses -lpthread

	
jogoui: JogoUI.c jogoui_utils.c
	gcc -o jogoui JogoUI.c jogoui_utils.c -lncurses -lpthread
	
clean:
	$(RM) bot motor jogoui

