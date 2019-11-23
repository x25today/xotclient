#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "x25.h"


static unsigned char buf[2050]; /* this is the send buffer */
static unsigned char rbuf[2050]; /* the reception buffer */
static int bufsz = 0, rbufsz=0, reverse_rbufsz=0;

void
reconnect_to(int *sockptr, char *host)
{
	int tries = 20;

	close(*sockptr);
	do {
		*sockptr = connect_to(host);
		if(*sockptr != -1)
			break;
		if(errno == ECONNREFUSED)
			sleep(15);
		else if(errno == ENETUNREACH || errno == ETIMEDOUT)
			sleep(7);
	}  while(tries--);

	if(!tries) {
		printf("CANNOT REACH GATEWAY %s\n", host);
		exit(1);
	}

}

int
connect_to (char *host)
{
	int sock, sockflags;
	struct sockaddr_in sin;
	struct hostent *he;
	

	sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	sockflags = fcntl(sock, F_GETFL);
	fcntl(sock, F_SETFL, sockflags|O_NONBLOCK);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(XOT_PORT);
	if((sin.sin_addr.s_addr = inet_addr(host))==-1)
	{
		if((he = gethostbyname(host))==NULL) {
				close(sock);
				return -1;
		}
		memcpy(&sin.sin_addr.s_addr, he->h_addr,4);
	}
	while((sockflags = connect(sock, (struct sockaddr *)&sin, sizeof(sin)))<0 &&
			(errno == EINPROGRESS || errno == EALREADY))
				;
	if(sockflags < 0)
	{
		close(sock);
		return -1;
	}
	return sock;
}


void
bsend(int sock, unsigned char byte)
{
	buf[4+bufsz] = byte;
	bufsz++;
}

void
xot_send(int sock)
{
	/* XOT header: version 0, length of the packet (<2^64) */
	buf[0] = buf[1] = 0;
	buf[2] = (bufsz >> 8) & 0xFF;
	buf[3] = bufsz& 0xFF;

	if(send(sock, buf, bufsz+4, 0)!=bufsz+4) {
		fprintf(stderr,"error writing..\n");
		exit(1);
	}
	bufsz = 0;

}

int
xot_recv(int sock)
{
	int count;	

        // this loop terminates for the first condition
	// when EGAIN is returned, for example. 
	// This will let us know that the socket buffer is empty:
	// because x.25 has no prefixed length, this is the only
	// signal we have finished reading.
	for(count = 0; read(sock, rbuf+count, 1)==1 &&  rbufsz<sizeof(rbuf); ++rbufsz, ++count)
			;

	if(errno != EAGAIN)
		return 0;
	return count;
	
	
}



unsigned char
brecv(int sock)
{
	time_t t;
	/* 15 second timeout */

	if(rbufsz==0) {
		reverse_rbufsz = 0;
		t = time(NULL)+ 15;
		while(!xot_recv(sock))
			if(time(NULL) >= t) {
			  	fprintf(stderr,"Timeout reading..\n");
				exit(1);
			}
		rbufsz -= 4;
	}
	rbufsz--;
	return rbuf[4+(reverse_rbufsz++)];
}

unsigned char
moredata()
{
	return (rbufsz!=0);
}
