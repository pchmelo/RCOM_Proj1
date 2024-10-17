typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    STOP,
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
    NOTHING_C
} C_TYPE;


//globla variables
#define FLAG 0x7E
#define A 0x03

#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B
#define C_FRAME0 0x00
#define C_FRAME1 0x80

unsigned char set_menssage[5] = {FLAG,A,C_SET,A^C_SET,FLAG};
unsigned char ua_menssage[5] = {FLAG,A,C_UA,A^C_UA,FLAG};

unsigned char rr0_menssage[5] = {FLAG,A,C_RR0,A^C_RR0,FLAG};
unsigned char rr1_menssage[5] = {FLAG,A,C_RR1,A^C_RR1,FLAG};

unsigned char rej0_menssage[5] = {FLAG,A,C_REJ0,A^C_REJ0,FLAG};
unsigned char rej1_menssage[5] = {FLAG,A,C_REJ1,A^C_REJ1,FLAG};

unsigned char disc_menssage[5] = {FLAG,A,C_DISC,A^C_DISC,FLAG};

