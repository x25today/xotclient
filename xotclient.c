#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include "x25.h"


static void
xot_error(int err)
{
	char c, d;
	int i;

	c = GET_CAUSE(err);
	d = GET_DIAG(err);
	printf("CLR C:%d D:%d\n", c, d);
	for(i = 0; errstr_cause[i].id != c && errstr_cause[i].id != -1; ++i)
			;
	if(errstr_cause[i].id != c)
		printf("Unknown cause code\n");
	else printf("%s\n", errstr_cause[i].str);
	for(i = 0; errstr_diag[i].id != d && errstr_diag[i].id != -1; ++i)
			;
	if(errstr_diag[i].id != d)
		printf("Unknown diag code\n");
	else    printf("%s\n", errstr_diag[i].str);
	
}



static void
pad_main(char *caller, char *called, char *gateway, int rev_charge)
{
	int c, err, i, maxselect;
	fd_set rfds;
	unsigned char p_s, p_r, to_ack;
	char buf[128];



	if((c = connect_to(gateway))<0)
	{
		perror("connect");
		exit(1);
	}
	if((err = x25_call(c, caller, called, rev_charge))!=CALL_OK)
	{
		xot_error(err);
		exit(1);
	}

	/* Some tty settings.. */
	tty_start();
	printf("COM\n");

	p_s = p_r = 0;
	to_ack = 0;
	while(1) {
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin), &rfds);
		if(to_ack < 2) {
		    FD_SET(c, &rfds);
		    maxselect = c+1;
		} else maxselect = fileno(stdin)+1;
		if(select(maxselect, &rfds, NULL, NULL, NULL)==-1)
			break;
		if(FD_ISSET(c, &rfds)) {
			PKINC(p_r);
			if(x25_in(c, &p_s, &p_r, &to_ack)) 
				break;
		}
		if(FD_ISSET(fileno(stdin), &rfds)) {
			if(!my_fgets(buf,sizeof(buf),stdin)) {
				x25_close(c);
				break;
			}
			/* Echo, but don't put line feed */
			for(i = 0; buf[i] != '\n' && buf[i] != '\r'; ++i)		
				lputchar(buf[i], 1);
			fflush(stdout);
			
			x25_out(c, buf, p_s, p_r);
			++to_ack;
			PKINC(p_s);
		} 
	}
	tty_stop();
	close(c);
}

static void
scan_main(char *gateway, char *source, int try_rev)
{
	char nua[128];
	int c, res;

	/* Connect */
	if((c = connect_to(gateway))<0) {
		perror("connect");
		exit(1);
	}

	/* Read nua */
	while(fgets(nua, sizeof(nua), stdin))
	{
		/* Strip \n and \r */
		nua[strlen(nua)-1] = 0;
		if(nua[strlen(nua)-1] == '\r')
			nua[strlen(nua)-1] = 0;
		if((res = x25_call(c, source, nua, 0))==CALL_OK) {
			printf("%s|NOREV|COM\n", nua);
			x25_close(c);
		} else 
			printf("%s|NOREV|CLR|%2.2u|%2.2u\n", nua, GET_CAUSE(res), GET_DIAG(res));
		reconnect_to(&c, gateway);
		if(try_rev && res == CALL_OK)
		{
			if((res = x25_call(c, source, nua, 1)) == CALL_OK) {
				printf("%s|REV|COM\n", nua);
				x25_close(c);
			} else 	
			printf("%s|REV|CLR|%2.2u|%2.2u\n", nua, GET_CAUSE(res), GET_DIAG(res));
			reconnect_to(&c, gateway);
		}
	}
}



int
main(int argc, char **argv)
{
	int c;
	char rev_charge;
	char *caller, *called;
	char *gateway;
	char mode;


	rev_charge = 0;
	caller = called = gateway = NULL;

	while((c = getopt(argc, argv, "rs:d:g:hSP"))!=EOF)
	switch (c) {
		case 's':
			caller = optarg;
			break;
		case 'd':
			called = optarg;
			break;
		case 'g':
			gateway = optarg;
			break;
		case 'r':
			rev_charge = 1;
			break;

		case 'S': 
			mode = MODE_SCAN;
			break;
		case 'P':
			mode = MODE_PAD;
			break;

		default:
			fprintf(stderr,"-s: sender, -d: destination\n -r: try (also)  reverse charge, -g: xot gateway\n-P: pad mode, -S: scan mode (default to SCAN mode)\n");
			exit(0);
	}

	if(!gateway || !caller || (mode == MODE_PAD && !called)) {
		fprintf(stderr, "-s, -g and (and -d in pad mode) are required\n");
		exit(0);
	}


	if(mode == MODE_SCAN)
		scan_main(gateway, caller, rev_charge);
	else    pad_main(caller, called, gateway, rev_charge);
	return 0;	

}
