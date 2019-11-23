#ifndef _X25_H
#define _X25_H

#define XOT_PORT 1998
#define X25_FAC_COMP_MARK               0x00
#define X25_FAC_REVERSE                 0x01
#define X25_FAC_THROUGHPUT              0x02
#define X25_FAC_CUG                     0x03
#define X25_FAC_CALLED_MODIF            0x08
#define X25_FAC_CUG_OUTGOING_ACC        0x09
#define X25_FAC_THROUGHPUT_MIN          0x0A
#define X25_FAC_EXPRESS_DATA            0x0B
#define X25_FAC_BILATERAL_CUG           0x41
#define X25_FAC_PACKET_SIZE             0x42
#define X25_FAC_WINDOW_SIZE             0x43
#define X25_FAC_RPOA_SELECTION          0x44
#define X25_FAC_TRANSIT_DELAY           0x49
#define X25_FAC_CALL_TRANSFER           0xC3
#define X25_FAC_CALLED_ADDR_EXT         0xC9
#define X25_FAC_ETE_TRANSIT_DELAY       0xCA
#define X25_FAC_CALLING_ADDR_EXT        0xCB
#define X25_FAC_CALL_DEFLECT            0xD1
#define X25_FAC_PRIORITY                0xD2

#define X29_CUD_LEN 4
#define X29_CUD_STR "\x01\x00\x00\x00"

#define X25_FLOW_RR 0x1
#define X25_FLOW_RNR 0x3
#define X25_FLOW_REJ 0x5


#define QBIT(u) ((u) & 0x80)
#define DBIT(u) ((u) & 0x40)
#define MBIT(u) ((u) & 0x08)
#define PRBITS(u) (((u)>>5) & 0x3)
#define PSBITS(u) ((u) & 0x3)
#define DATABIT(u) (!((u) & 0x1))

int connect_to(char *);
void reconnect_to(int *, char *);
void bsend(int, unsigned char);
unsigned char brecv(int);

void xot_send(int);
int xot_recv(int);
unsigned char moredata();

void x25_header(int, char, char, char, char, unsigned char, unsigned char);
int x25_call(int, char *, char *, int);

int x25_in(int, unsigned char *, unsigned char *, unsigned  char *);
void x25_out(int, unsigned char *, unsigned char, unsigned char);
void x25_sendrr(int, unsigned char);
void x25_close(int);

void lputchar(unsigned char, unsigned char);
void tty_start();
void tty_stop();
void pad_control(unsigned char, unsigned char);
char *my_fgets(unsigned char *, int, FILE *);



#define CALL_OK -1
#define MAKE_ERROR(c, d) (((c)<<16)|(d))
#define GET_CAUSE(e) (((e >> 16) & 0xFF))
#define GET_DIAG(e)  (((e) & 0xFF))

#define PKT_TYPE_CALL_REQ 0xB /* call request */
#define PKT_TYPE_CALL_ACC 0xF /* call accepted */
#define PKT_TYPE_CALL_CLR 0x13 /* call clear request */
#define PKT_TYPE_CALL_CLC 0x17 /* call clear confirmation */
#define PKT_TYPE_RESET_REQ 0x1B

#define X25_MOD_8 0x01
#define X25_MOD_128  0x10

#define PKINC(a) ((a) = ((a) + 1)%8)
#define PKDEC(a) (((a)==0)?(a)=7:((a) = ((a) - 1)%8))

struct error_entry {
	char id;
	char *str;
};

static struct error_entry errstr_cause[] =  {
	{ 0, "DTE Originated" },  { 1, "Number Busy" }, { 3, "Invalid Facility Requested" },
	{ 5, "Network Congestion"}, { 9, "Out Of order" }, { 11, "Access Barred" },
	{ 13, "Not obtainable" }, { 17, "Remote procedure error" }, { 19, "Local procedure error" },
	{ 21, "RPOA out of order" }, { 25, "Reverse charging not accepted" }, 
	{ 33, "Incompatible destination" }, { 41, "Fast select not accepted" },
	{ 57, "Ship absent" }, {-1, NULL }};

static struct error_entry errstr_diag[] = {
	{ 0, "No additional information" }, { 1, "Invaild p(s)" }, { 2, "Invalid p(r)" },
	{ 16, "Packet type Invalid" }, { 32, "Packet not allowed" }, {33, "Unidentifiable packet"},
	{ 34, "Call on one-way logical channel" }, { 35, "Invalid packet type on PVC" },
	{ 36, "Packet to unassigned LCN" }, { 37, "Reject not subscribed to" },
	{ 38, "Packet too short" }, { 39, "Packet too long" }, { 40, "Invalid GFI" },
	{ 41, "Restart or registration packet with nonzero LCI" }, 
	{ 42, "Packet type not compatible with facility" },
	{ 43, "Unauthorized interrupt confirmation" }, { 44, "Unauthorized interrupt" },
	{ 45, "Unauthorized reject" }, { 48, "Timer expired" }, 
	{ 64, "Call setup clearing or registration problem" },
	{ 65, "Facility code not allowed" }, { 66, "Facility parameter not allowed" },
	{ 67, "Invalid called address" }, { 68, "Invalid calling address" }, 
	{ 69, "Invalid facility length" }, { 70, "Incoming call barred" },
	{ 71, "No logical channel avaiable" }, { 72, "Call callision (kaboom!)" },
	{ 73, "Duplicate facility requested" }, { 74, "Nonzero address length" },
	{ 75, "Nonzero facility length" }, { 80, "Miscellaneous" },
	{ 84, "NUI problem" }, { 112, "International problem" },
	{ 113, "Remote network problem" }, { 114, "International protocol problem" },
	{ 115, "international link outoforder" }, { 116, "Internation link busy" },
	{ 117, "Transit network facility problem" }, { 118, "Remote network facility problem" },
	{ 119, "International Routing Problem" }, { 120, "Temporary routing problem" },
	{ 121, "Unknown called DNIC" }, { 122, "Maintenance Action" }, { -1, NULL } };

	


#define MODE_SCAN 0
#define MODE_PAD 1


#define LOGFILE "logfile"
#endif
