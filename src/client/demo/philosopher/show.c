
#include "philos_msg.h"

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#define WIDTH  10
#define HEIGHT 4
#define TIME_OUT 1000

#define MAX_PHIOLS_NUM 8

typedef struct {
    philosopher* philos;
    int x;
    int y;
}philos_show;

philos_show philos_show_pool[MAX_PHILOS_NUM];
static int phi_count = 0;

char* status_pool[] = {"THINK", "EAT", "BUSY"};
void show_addphilos(philosopher* phi)
{
    if (!phi) return;
    if (phi_count >= MAX_PHILOS_NUM) return;

    philos_show* s = &philos_show_pool[phi_count++];
    s->philos = phi;
}

void draw_board(WINDOW *win, int starty, int startx, int lines, int cols, 
	   int tile_width, int tile_height)
{	int endy, endx, i, j;
	
	endy = starty + lines * tile_height;
	endx = startx + cols  * tile_width;
	
	for(j = starty; j <= endy; j += tile_height)
		for(i = startx; i <= endx; ++i)
			mvwaddch(win, j, i, ACS_HLINE);
	for(i = startx; i <= endx; i += tile_width)
		for(j = starty; j <= endy; ++j)
			mvwaddch(win, j, i, ACS_VLINE);
	mvwaddch(win, starty, startx, ACS_ULCORNER);
	mvwaddch(win, endy, startx, ACS_LLCORNER);
	mvwaddch(win, starty, endx, ACS_URCORNER);
	mvwaddch(win, 	endy, endx, ACS_LRCORNER);
	for(j = starty + tile_height; j <= endy - tile_height; j += tile_height)
	{	
        mvwaddch(win, j, startx, ACS_LTEE);
		mvwaddch(win, j, endx, ACS_RTEE);	
		for(i = startx + tile_width; i <= endx - tile_width; i += tile_width)
			mvwaddch(win, j, i, ACS_PLUS);
	}

    int idx = 0;
    philos_show_pool[idx].x = startx;
    philos_show_pool[idx].y = starty;
	for(i = startx + tile_width; i <= endx - tile_width; i += tile_width)
	{	
        idx ++;
        mvwaddch(win, starty, i, ACS_TTEE);
		mvwaddch(win, endy, i, ACS_BTEE);

        philos_show_pool[idx].x = i;
        philos_show_pool[idx].y = starty;
	}
	wrefresh(win);
}

void draw_philos_info(WINDOW* win, int num)
{
    int i;
    for (i=0; i < num; ++i){
        mvwaddstr(win, philos_show_pool[i].y+1, philos_show_pool[i].x+1, philos_show_pool[i].philos->name);
	    mvwprintw(win,philos_show_pool[i].y+2, philos_show_pool[i].x+1, "%s", status_pool[philos_show_pool[i].philos->status]);
    }
}

void refresh_screen()
{
    wclear(stdscr);
    
	curs_set(FALSE);
    draw_board(stdscr, LINES/2, COLS/phi_count, 1, phi_count, WIDTH, HEIGHT);
    draw_philos_info(stdscr, phi_count);

    //attron(A_REVERSE);
    box(stdscr, '|', '-');
    //attroff(A_REVERSE);
    wrefresh(stdscr);
}

