#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>

const char *tips[] = {
	"%c[33mL4 %c[31mT%c[32me%c[33mt%c[34mr%c[35mi%c[36ms%c[39m",
	"http://l4os.ru/tetris",
	"", 
	"Space - drop figure",
	"Left - shift left",
	"Right - shift ritght",
	"Up - rotate clockwise",
	"Down - rotate counterwise",
	"Q - quit game",
	"P - pause"
};

#ifdef LINUX
	#define TIOCGETA	TCGETA
	#define TIOCSETA	TCSETA
#endif

#if 0
	#define GLASS_WIDTH		8
	#define GLASS_HEIGHT	10
#else
	#define GLASS_WIDTH		10
	#define GLASS_HEIGHT	14
#endif

typedef struct { int x, y; } cell_t;

char		glass[GLASS_WIDTH][GLASS_HEIGHT]; // Стакан
cell_t		figure[4];	// Фигура
int			score;		// Счёт игры
int			speedup;	// Скорость падения фигур
int			speed;		// Время текущей задержки
int			level;		// Уровень игры
int			current;	// Индекс текущей фигуры
int			next;		// Индекс следующей фигуры
int			rotate;		// Угол вращения
int			first_row;	
int			column;
struct timeval	line_start_time; 
char		commands[80*20];
int			cmd_position;

#define COMMAND_PREFIX		&commands[cmd_position], sizeof(commands) - cmd_position, 
#define	LEFT				((80 - GLASS_WIDTH)>>1)
#define	TOP					3

void show_help(void)
{
	int i;
	char	str[300];
	for(i=0; i<sizeof(tips)/sizeof(tips[0]); i++)
	{
		snprintf( str, sizeof(str), tips[i], 27, 27, 27, 27, 27, 27, 27, 27 );
		cmd_position += snprintf( COMMAND_PREFIX "%c[%d;%dH%s", 27, 6+i, 1, str );
	}
}

// Создаёт фигуру
void create_figure(int type, int x, int y, int angle)
{
	angle = angle % 4;

	switch(type)
	{
		case 0: // Квадрат
			figure[0].x = x - 1;		figure[0].y = y;
			figure[1].x = x;			figure[1].y = y;
			figure[2].x = x - 1;		figure[2].y = y + 1;
			figure[3].x = x;			figure[3].y = y + 1;
			break;

		case 1: // Треуголник
			switch(angle)
			{
			case 0:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x;		figure[3].y = y + 1;
				break;
			case 1:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x;		figure[3].y = y + 1;
				break;
			case 2:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x;		figure[3].y = y - 1;
				break;
			case 3:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x;		figure[2].y = y + 1;
				figure[3].x = x - 1;	figure[3].y = y;
				break;
			}
			break;

		case 2: // Угол правый
			switch(angle)
			{
			case 0:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x + 1;	figure[3].y = y + 1;
				break;
			case 1:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x;		figure[2].y = y + 1;
				figure[3].x = x - 1;	figure[3].y = y + 1;
				break;
			case 2:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x - 1;	figure[3].y = y - 1;
				break;
			case 3:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x;		figure[2].y = y + 1;
				figure[3].x = x + 1;	figure[3].y = y - 1;
				break;
			}
			break;

		case 3: // Угол левый
			switch(angle)
			{
			case 0:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x - 1;	figure[3].y = y + 1;
				break;
			case 1:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x;		figure[2].y = y + 1;
				figure[3].x = x + 1;	figure[3].y = y + 1;
				break;
			case 2:
				figure[0].x = x - 1;	figure[0].y = y + 1;
				figure[1].x = x;		figure[1].y = y + 1;
				figure[2].x = x + 1;	figure[2].y = y + 1;
				figure[3].x = x + 1;	figure[3].y = y;
				break;
			case 3:
				figure[0].x = x;		figure[0].y = y - 1;
				figure[1].x = x + 1;	figure[1].y = y - 1;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x + 1;	figure[3].y = y + 1;
				break;
			}
			break;

		case 4: // Линия
			switch(angle)
			{
			case 0:
			case 2:
				figure[0].x = x - 1;	figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y;
				figure[3].x = x + 2;	figure[3].y = y;
				break;
			case 1:
			case 3:
				figure[0].x = x;		figure[0].y = y - 2;
				figure[1].x = x;		figure[1].y = y - 1;
				figure[2].x = x;		figure[2].y = y;
				figure[3].x = x;		figure[3].y = y + 1;
				break;
			}
			break;

		case 5: // Сдвиг 1
			switch(angle)
			{
			case 0:
			case 2:
				figure[0].x = x-1;		figure[0].y = y;
				figure[1].x = x;		figure[1].y = y;
				figure[2].x = x;		figure[2].y = y + 1;
				figure[3].x = x+1;		figure[3].y = y + 1;
				break;
			case 1:
			case 3:
				figure[0].x = x;		figure[0].y = y;
				figure[1].x = x + 1;	figure[1].y = y;
				figure[2].x = x + 1;	figure[2].y = y - 1;
				figure[3].x = x;		figure[3].y = y + 1;
				break;

			}
			break;

		case 6: // Сдвиг 2
			switch(angle)
			{
			case 0:
			case 2:
				figure[0].x = x;		figure[0].y = y;
				figure[1].x = x+1;		figure[1].y = y;
				figure[2].x = x-1;		figure[2].y = y + 1;
				figure[3].x = x;		figure[3].y = y + 1;
				break;
			case 1:
			case 3:
				figure[0].x = x - 1;	figure[0].y = y - 1;
				figure[1].x = x - 1;	figure[1].y = y;
				figure[2].x = x;		figure[2].y = y;
				figure[3].x = x;		figure[3].y = y + 1;
				break;
			}
			break;

		default:
			printf("Unknown figure\n");
			break;
	}
}



// Проверяет, пересекается ли фигура
int is_crossing()
{
	int i;
	for( i=0; i<sizeof(figure)/sizeof(figure[0]); i++ )
	{
		int x = figure[i].x;
		int y = figure[i].y;
		if( x < 0 || x >= GLASS_WIDTH || glass[x][y] != 0 ) return -1;
	}
	return 0;
}

// Рисует стакан
void draw_glass()
{
	int		i, j;
	
	cmd_position += snprintf( COMMAND_PREFIX "%c[2J", 27);
	for( j=0; j <GLASS_HEIGHT; j++ )
	{
		cmd_position += snprintf( COMMAND_PREFIX "%c[%d;%dH|", 27, TOP+j, LEFT );
		for( i=0; i < GLASS_WIDTH; i++ ) 
		{
			if(glass[i][j] == 0)
				cmd_position  += snprintf( COMMAND_PREFIX " .");
			else
				cmd_position  += snprintf( COMMAND_PREFIX "%c[42m  %c[0m", 27, 27);
		}
		cmd_position  += snprintf( COMMAND_PREFIX "|");
	}
	cmd_position += snprintf( COMMAND_PREFIX "%c[%d;%dH", 27, TOP+j, LEFT );
	for( i = 0; i < GLASS_WIDTH + 1; i++ ) cmd_position  += snprintf( COMMAND_PREFIX "==");
	
	level = 1999 / (speedup+100);
	cmd_position  += snprintf(  COMMAND_PREFIX 
		"%c[44m%c[1;1H+---------------+"
		"%c[2;1H| Level: %6d |"
		"%c[3;1H| Score: %6d |"
		"%c[4;1H+---------------+%c[40m",
		27,  27,
		27,level, 
		27, score,
		27, 27);

	show_help();
	puts(commands);
}

// Очищает стакан
void clear_glass()
{
	int i, j;

	for( j=0; j < GLASS_HEIGHT; j++ )
		for( i=0; i < GLASS_WIDTH; i++ ) 
			glass[j][i] = 0;
}

// Выбирает случайную фигуру
int get_random_figure() 
{ 
	int x = rand() % 7;
	return x;
}

// Очищает фигуру
void clear_figure()
{
	cmd_position += snprintf( COMMAND_PREFIX
		"%c[%d;%dH .%c[%d;%dH .%c[%d;%dH .%c[%d;%dH .\n",
		27, figure[0].y + TOP, (2 * figure[0].x) + 1 + LEFT,
		27, figure[1].y + TOP, (2 * figure[1].x) + 1 + LEFT,
		27, figure[2].y + TOP, (2 * figure[2].x) + 1 + LEFT,
		27, figure[3].y + TOP, (2 * figure[3].x) + 1 + LEFT
		);
}

// Рисует фигуру
void draw_figure()
{
	cmd_position += snprintf( COMMAND_PREFIX
		"%c[43m%c[%d;%dH  %c[%d;%dH %c%c[%d;%dH  %c[%d;%dH  %c[0m\n",
		27, 
		27, figure[0].y + TOP, (2 * figure[0].x) + 1 + LEFT,
		27, figure[1].y + TOP, (2 * figure[1].x) + 1 + LEFT, rotate + '0',
		27, figure[2].y + TOP, (2 * figure[2].x) + 1 + LEFT,
		27, figure[3].y + TOP, (2 * figure[3].x) + 1 + LEFT,
		27
		);
	puts(commands);
}

void draw_next()
{
	create_figure( next, column, first_row, rotate );

	cmd_position += snprintf( COMMAND_PREFIX
		"%c[%d;%dHNext block%c[43m%c[%d;%dH  %c[%d;%dH  %c[%d;%dH  %c[%d;%dH  %c[0m\n",
		27, TOP, 4 + LEFT + (GLASS_WIDTH*2), 27, 
		27, figure[0].y + TOP + 1, (2 * figure[0].x) + LEFT + (GLASS_WIDTH*2) - 2,
		27, figure[1].y + TOP + 1, (2 * figure[1].x) + LEFT + (GLASS_WIDTH*2) - 2,
		27, figure[2].y + TOP + 1, (2 * figure[2].x) + LEFT + (GLASS_WIDTH*2) - 2,
		27, figure[3].y + TOP + 1, (2 * figure[3].x) + LEFT + (GLASS_WIDTH*2) - 2,
		27
		);

	create_figure( current, column, first_row, rotate );

	puts(commands);
}

// Сдвиг вниз
int step_down()
{
	int		locked = 0;
	int		i,j,k;
	int		glass_full = 0;
	
	for( i=0; i<sizeof(figure)/sizeof(figure[0]); i++ )  
	{
		if( figure[i].y < 0 ) continue;
		if( glass[figure[i].x][figure[i].y+1] != 0) 
		{
			locked = -1;
			break;
		}

		if( figure[i].y == GLASS_HEIGHT - 1 )
		{
			locked = -1;
			break;
		}
	}

	if( ! locked )
	{
		clear_figure();
		first_row++;
		for( i=0; i<sizeof(figure)/sizeof(figure[0]); i++ ) figure[i].y++;
		draw_figure();
		if( speed > 50 ) speed = speedup;
	}
	else
	{
		// Fill active final position
		for( i=0; i<sizeof(figure)/sizeof(figure[0]); i++ ) glass[figure[i].x][figure[i].y] = 1;
		
		// Throw filled lines
		for(j=1; j<GLASS_HEIGHT; j++)
		{
			int fill_line = 1;
			for( i = 0; i<GLASS_WIDTH; i++ )
				if( glass[i][j] == 0 )
				{
					fill_line = 0;
					break;
				}
			if( fill_line )
			{
				for( k=j; k>0; k--)
				{
					for( i = 0; i<GLASS_WIDTH; i++ )
					{
						glass[i][k] = glass[i][k-1];
					}
				}
				for( i = 0; i<GLASS_WIDTH; i++ ) glass[i][0] = 0;
			}
		}
		draw_glass();
		
		// Check the galss full
		for(i=0; i<GLASS_WIDTH; i++)
		{
			if( glass[i][0] != 0 ) glass_full = 1;
		}

		speed = speedup;
		first_row = 0;
		rotate = 0;					// Всегда с одной позиции
		column = GLASS_WIDTH / 2;
		current = next;
		next = get_random_figure();
		create_figure( current, column, first_row, rotate );
		draw_next();
		
		score += 10;
		// Чуть увеличиваем скорость после каждой упавшей фигуры
		if( speedup > 50 ) speedup--;
	}
	return glass_full;
}

// Главная функция
int main(int argc, char * argv[], char**envp)
{
    int				nStatus;
	int				ReadCounter;
	int				play_game = 1;
	struct pollfd	pfd;
	struct timeval	line_current_time; 
	unsigned char	keycode[10];
	int				i,t;
	long int		diff;		// Время до таймаута
#ifdef LINUX
	struct termio tms;
#else
	struct termios tms;
#endif

	nStatus = ioctl( 0, TIOCGETA, &tms );
	if( nStatus < 0) { perror("Unable TIOCGETA"); exit(1); }
	tms.c_lflag ^=  ICANON | ECHO;
	nStatus = ioctl(0, TIOCSETA, &tms );
	if( nStatus < 0) { perror("Unable TIOCSETA"); exit(1); }

	pfd.fd = 0;
	pfd.events = POLLIN;
	pfd.revents = 0;

	speedup = 1000; // Начальная задержка в милисекундх
	rotate = 0;
	first_row = 0;
	column = GLASS_WIDTH / 2;
	current = get_random_figure();
	clear_glass();
	draw_glass();
	cmd_position = 0;
	create_figure( current, column, first_row, rotate );
	draw_next();

	gettimeofday( &line_start_time, NULL);
	speed = speedup;

	while(play_game)
	{
		draw_figure();
		cmd_position = 0;
		nStatus = poll( &pfd, 1, speed);
		
//		printf("%c[%d;%dH   %d %ld  \n", 27, TOP+7, 4 + LEFT + (GLASS_WIDTH*2), speed, diff );

		switch( nStatus )
		{
		case 1:		/* key pressed */
			gettimeofday( &line_current_time, NULL);
			diff = (((line_current_time.tv_usec + 1000000 * line_current_time.tv_sec) - (line_start_time.tv_usec + 1000000 * line_start_time.tv_sec))  % 1000000) / 1000;
			if(speed > (51 + diff) ) speed -= diff;
			ReadCounter = read( 0, keycode, sizeof(keycode) );

			if( tolower(keycode[0]) == 'q') { play_game = 0; continue; }
			if( tolower(keycode[0]) == 'p' ) { nStatus = poll( &pfd, 1, -1 );  continue; }
			if( keycode[0] == ' ') 
			{ 
				speed = 50; 
				if( step_down() != 0 )
				{
					play_game = 0;
				}
				continue; 
			}
			if( keycode[0] == 27  && keycode[1] == 91 )
			{
				int lock = 0;
			    switch(keycode[2])
			    {
				case 68: // left
					for(i=0; i<4; i++) 
					{
						if(figure[i].x == 0) 
						{
							lock = -1;
							break;
						}
						else if( glass[figure[i].x-1][figure[i].y] != 0 )
						{
							lock = -1;
							break;
						}
					}
					if( ! lock )
					{
						clear_figure();
						column--;
						create_figure( current, column, first_row, rotate );
						draw_figure();
					}
					break;

				case 65: // down
					clear_figure();
					rotate = (rotate + 1) % 4;
					create_figure( current, column, first_row, rotate );
					if( ! is_crossing() ) 
					{
						draw_figure();
					}
					else
					{
						rotate = ((unsigned int) (rotate - 1)) % 4;
						create_figure( current, column, first_row, rotate );
					}
					break;

				case 66: // up
					clear_figure();
					rotate = ((unsigned int) (rotate - 1)) % 4;
					create_figure( current, column, first_row, rotate );
					if( ! is_crossing() ) 
					{
						draw_figure();
					}
					else
					{
						rotate = (rotate + 1) % 4;
						create_figure( current, column, first_row, rotate );
					}
					break;

				case 67: // right
					for(i=0; i<4; i++) 
					{
						if(figure[i].x == GLASS_WIDTH - 1 )
						{
							lock = -1;
							break;
						}
						else if( glass[figure[i].x+1][figure[i].y] != 0 )
						{
							lock = -1;
							break;
						}
					}
					if( ! lock )
					{
						clear_figure();
						column++;
						create_figure( current, column, first_row, rotate );
						draw_figure();
					}
					break;

				default:  // Ну ты и лох, Лёха, так делать нельзя
					goto L1;
			    }

			}
			else // Об этих кнопках мы пока не знаем
			{
L1:
				printf("%c[%d;%dH Read %d bytes\n", 27, 20, 3, ReadCounter );
				for(i=0; i<ReadCounter; i++) printf( "%3d", keycode[i] );
				printf("\n");
			}
			break;

		case 0:		/* timeout */
			gettimeofday( &line_start_time, NULL);
			if( step_down() != 0 )
			{
				play_game = 0;
			}
			continue;

		case -1:	/* error */
			perror("Unable watch keyboard");
			play_game = 0;
			continue;
		}
	}
	printf("\nGAME OVER!\n");
	
	nStatus = ioctl( 0, TIOCGETA, &tms );
	if( nStatus < 0) { perror("Unable TIOCGETA"); exit(1); }
	tms.c_lflag |=  ICANON | ECHO;
	nStatus = ioctl(0, TIOCSETA, &tms );
	if( nStatus < 0) { perror("Unable TIOCSETA"); exit(1); }

	return 0;
}
