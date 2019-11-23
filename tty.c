#include <stdio.h>
#include <sys/types.h>
#include <curses.h>
#include <stdlib.h>
#include "x25.h"


static FILE *logfile  = NULL;
static WINDOW *scr;
static int doecho = 0;

static int
startlog() {
	if(logfile)
		return 1;
	logfile = fopen(LOGFILE, "a");
	return (logfile != NULL);
}

static void
stoplog()
{
	if(logfile)
		fclose(logfile);
	logfile = NULL;
}

void
lputchar(unsigned char c, unsigned char from_input)
{
	if(logfile)
		fputc(c, logfile);
	if(doecho || !from_input)
		putchar(c);

}

char *
my_fgets(unsigned char *string, int bufsz, FILE *fp)
{
	int i, b;

	for(i = 0; i < bufsz-1; ++i)
	{
		b = getch();

		if(b == KEY_F(2)) 
			startlog();
		else if(b == KEY_F(3)) 
			stoplog();
		else if(b == KEY_F(10)) {
			printf("EXITING!\n");
			return NULL;
		}  else {
			string[i] = (unsigned char)(b & 0xFF);
			if(string[i] == '\n') {
				++i;
				break;
			}
		}
	}
	string[i] = 0;
	return string;
}

void
tty_start() {

	scr = initscr();
	keypad(scr, 1);
	noecho();
	wclear(scr);
	wrefresh(scr);
}
void
tty_stop()
{
	stoplog();
	echo();
	keypad(scr, 0);
	endwin();
}

void
pad_control(unsigned char code, unsigned char param)
{
	switch(code) {
		case 0x02: /* ECHO */
			doecho = param;
			break;
		default:
			/* not supported or not implemented */
			break;
	}
}

