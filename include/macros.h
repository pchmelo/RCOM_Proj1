#ifndef MACROS_H
#define MACROS_H

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    STOP_BIG,
    STOP_SMALL,
    BCC_OK,
    DATA,
    END
} State;

typedef enum{
    SET,
    UA,
    RR0,
    RR1,
    REJ0,
    REJ1,
    DISC,
    FRAME1,
    FRAME0,
    ERROR,
    NOTHING_C
} C_TYPE;


//globla variables
#define FLAG     0x7E
#define A_Tx        0x03
#define A_Rx     0x01

#define C_SET    0x03
#define C_UA     0x07
#define C_RR0    0xAA
#define C_RR1    0xAB
#define C_REJ0   0x54
#define C_REJ1   0x55
#define C_DISC   0x0B
#define C_FRAME0 0x00
#define C_FRAME1 0x80

unsigned char set_menssage[5] = {FLAG,A_Tx,C_SET,A_Tx^C_SET,FLAG};
unsigned char ua_menssage_tx[5] = {FLAG,A_Tx,C_UA,A_Tx^C_UA,FLAG};
unsigned char ua_menssage_rx[5] = {FLAG,A_Rx,C_UA,A_Rx^C_UA,FLAG};

unsigned char rr0_menssage[5] = {FLAG,A_Tx,C_RR0,A_Tx^C_RR0,FLAG};
unsigned char rr1_menssage[5] = {FLAG,A_Tx,C_RR1,A_Tx^C_RR1,FLAG};

unsigned char rej0_menssage[5] = {FLAG,A_Tx,C_REJ0,A_Tx^C_REJ0,FLAG};
unsigned char rej1_menssage[5] = {FLAG,A_Tx,C_REJ1,A_Tx^C_REJ1,FLAG};

unsigned char disc_menssage[5] = {FLAG,A_Tx,C_DISC,A_Tx^C_DISC,FLAG};
unsigned char disc_menssage_rx[5] = {FLAG,A_Rx,C_DISC,A_Rx^C_DISC,FLAG};

int llopenRx();
int llopenTx();
unsigned char* read_aux(int *readenBytes, bool alarm);
int write_aux(unsigned char *mensage, int numBytes);
unsigned char* suffing_encode(const unsigned char *buf, int bufSize, int *newBufSize);
unsigned char* stuffing_decode(unsigned char *buf, int bufSize, int *newBufSize);
char calculate_BCC2(const unsigned char *buf, int bufSize);
int mount_frame_menssage(int numBytesMenssage, unsigned char *buf, unsigned char *frame, unsigned char bb2);
void final_check();
void debug_write(unsigned char *mensage, int numBytes);
void debug_read(unsigned char *mensage, int numBytes);
int c_check(unsigned char byte);
int handle_llwrite_reception();
int handle_llread_reception(unsigned char *buf, int bufSize);

#endif