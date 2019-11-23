#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "x25.h"

int
x25_call(int sock, char *source, char *dest, int has_rev)
{
	int s_len, d_len;
	int i;
	unsigned char b;


	d_len = strlen(dest);
	s_len = strlen(source);
	/* CALL REQUEST */
	x25_header(sock, 0, 0, X25_MOD_8, 0, 1, PKT_TYPE_CALL_REQ);

	/* calling address and called address length */
	b = (s_len & 0xF) << 4 | (d_len & 0xF); bsend(sock, b); 

	/* the two padded addresses */
	for(i = 0; d_len-i > 1; i+=2) {
	        b = (dest[i]-'0') << 4;
		b |= (dest[i+1]-'0');
		bsend(sock, b);
	}
	if(d_len-i == 1)    
	      bsend(sock, ((dest[i]-'0') << 4) | (source[0]-'0'));

	for(i = ((d_len-i) % 2); s_len-i > 1; i+=2)
		bsend(sock, (((source[i]-'0') << 4) | (source[i+1]-'0')));
	if(s_len-i == 2)
		bsend(sock, (((source[i]-'0') << 4) | (source[i+1]-'0')));
	else  if(s_len-i == 1)  bsend(sock, ((source[i]-'0') << 4));


	/* facilities: we need packet and window size, and eventually reverse_charge.
	   each facility has calling and called part */ 

	i = 6;
	if(has_rev) i+=2;

	/* facilities length */
	bsend(sock, i&0xFF);

	/* the window size */
	bsend(sock, X25_FAC_WINDOW_SIZE);
	bsend(sock, 2);/* from our dte */
	bsend(sock, 2);/* from remote dte */

	/* the packet size */
	bsend(sock, X25_FAC_PACKET_SIZE);
	bsend(sock, 0x07); /* 2^7 -> 128 byte per datapacket, from our dte */
	bsend(sock, 0x07); /* 2^7 -> 128 byte per datapacket, from remote dte */

	/* if neecessary, reverse charge */
	if(has_rev) {
		bsend(sock, X25_FAC_REVERSE);
		bsend(sock, 1);
	}

	/* send the CUD: we want X.29! */
	for(i = 0; i < X29_CUD_LEN; ++i)
		bsend(sock, X29_CUD_STR[i]);

	/* really send */
	xot_send(sock);

	/* NOW WAIT FOR ANSWER */	

	brecv(sock); /* GFI, ignored on XOT */
	brecv(sock); /* channel number, ignored on XOT */

	b = brecv(sock); /* the packet type, this can change our life */
	if(b ==  PKT_TYPE_CALL_ACC) /* call accepted */ {
		brecv(sock); /* the copy of the original packet */
		for(i = 0; i < (s_len+d_len)/2; i++)
			brecv(sock);
		if((s_len+d_len) % 2)
			brecv(sock);	
		b = brecv(sock); /* facilities length.. */
		while(b--)
			brecv(sock);
		while(moredata()) {
			   b = brecv(sock);
			   lputchar(b, 0);
			}
		return CALL_OK;
	} else if(b == PKT_TYPE_CALL_CLR) { /* call not accepted */
		/* send back clear confirmation */
		b = brecv(sock); 
		i = brecv(sock);
		x25_header(sock, 0, 0, X25_MOD_8, 0, 1, PKT_TYPE_CALL_CLC);
		xot_send(sock);
		return MAKE_ERROR(b, (i&0xFF));
	}

	return 0; /* stupid gcc */
}

void
x25_sendrr(int sock, unsigned char pr)
{
	x25_header(sock, 0, 0, X25_MOD_8, 0, 1, (pr<<5)|(X25_FLOW_RR));
	xot_send(sock);
}

void
x25_header(int sock, char q, char d, char mod, char lgn, unsigned char lcn, unsigned char ptype)
{
	unsigned char buf[3];

	buf[0] = ((q&1)<<7) | ((d&1)<<6) | ((mod&3)<<4) | (lgn&0xF);
	buf[1] = lcn;
	buf[2] = ptype;

	bsend(sock, buf[0]);
	bsend(sock, buf[1]);
	bsend(sock, buf[2]);
	
}

void
x25_out(int sock, unsigned char *tline, unsigned char ps, unsigned char pr)
{

	/* Make the header */
	/* P(R), P(S), No M-bit */
	x25_header(sock, 0, 0, X25_MOD_8, 0, 1, (pr << 5) | (ps<<1));

	while(*tline)
		bsend(sock, *tline++);
	
	xot_send(sock);
}

int
x25_in(int sock, unsigned char *psp, unsigned char *prp, unsigned char *to_ack)
{
	unsigned char head[3];
	int i;
	unsigned char a, b;

	/* First, get the header */
	for(i = 0; i < 3; ++i)
		head[i] = brecv(sock);

	if(QBIT(head[0])) {  /* Control packet, 
			        these are referred as TTY control */
		 brecv(sock); /* ITU-T x.28 has code 0x02 */
		while(moredata())  {
			a = brecv(sock);
			b = brecv(sock);
			pad_control(a, b);
		}
		x25_sendrr(sock, *prp);
		return 0;
	}



	/* If it is data.. let's print it out */
	if(DATABIT(head[2])) {
	   if(MBIT(head[2])) 
		; /* let's expect some more data 
		     XXX Must P(S) be decremented ??? */
	   while(moredata())
		lputchar(brecv(sock), 0);
	   fflush(stdout);	
	   /* Send a receiver ready: we have data! */
	   x25_sendrr(sock, *prp);
	   return 0;
	}

	/* Also non-Q bit RR exists. RR means "packet acknowledged, go on trasmitting"
	   Missing RRs before transmit will generate Reset Request from the called system */
	if((head[2] & 0x1F) == X25_FLOW_RR) {
		PKDEC(*prp);
		(*to_ack)--;
		return 0;
	}


	/* Reset Request, get Cause and Diagnosis */
	if(head[2] == PKT_TYPE_RESET_REQ)
	{
		head[0] = brecv(sock);
		fprintf(stderr, "RESET REQUEST: %u ", head[0]);
		if(moredata()) {
			head[1] = brecv(sock);
			fprintf(stderr, "(diag code = %u)", head[1]);
		}
		fprintf(stderr, "\n");
		return 0;
	}

	/* Clear request: end of file */
	if(head[2] == PKT_TYPE_CALL_CLR) {
	         x25_header(sock, 0, 0, X25_MOD_8, 0, 1, PKT_TYPE_CALL_CLC);
		 xot_send(sock);
		 return 1;
	}
	
	fprintf(stderr, "Unhandled packet : %u\n", head[2]);
	return 0;
	
	
}
	

void
x25_close(int sock)
{
	x25_header(sock, 0, 0, X25_MOD_8, 0, 1, PKT_TYPE_CALL_CLR);
	bsend(sock, 0); bsend(sock, 136);
	xot_send(sock);
}
