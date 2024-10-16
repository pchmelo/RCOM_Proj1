// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

//Global Variables
#define BUF_SIZE 255

unsigned char buf[BUF_SIZE] = {0};

State state = END;
C_TYPE control = NOTHING_C;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0){
        return -1;
    }

    // TODO
    if(connectionParameters.role == LlRx){
        // Wait for a SET then ...
        // Send UA
    } else {
        // Send SET (write)
        // Wait UA (read)
    }

    
    return 1;
}

int llopenRx(){

}

int llopenTx(){
    sendSET(); // Send SET
    unsigned char packet[5] = {0};
    llread(*packet);    // Read UA
    checkUA(packet);    // Check UA
}

int sendSET(){
    unsigned char set[5] = {0};
    
    set[0] = FLAG;
    set[1] = A;
    set[2] = C_SET;
    set[3] = set[1] ^ set[2];
    set[4] = FLAG;

    if(writeBytesSerialPort(set, 5) == -1){
        perror("Error sending SET");
        return -1;
    }

    return 1;
}

int sendUA(){
    unsigned char ua[5] = {0};

    ua[0] = FLAG;
    ua[1] = A;
    ua[2] = C_UA;
    ua[3] = ua[1] ^ ua[2];
    ua[4] = FLAG;

    if(writeBytesSerialPort(ua, 5) == -1){
        perror("Error sending UA");
        return -1;
    }

    return 1;
}



////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet){
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}

int stateMachineHandler(unsigned char byte){
    switch (state){
    case START:
        if(byte == FLAG){
            state = FLAG_RCV;
        }
        break;
    
    case FLAG_RCV:
        if(byte == A){
            state = A_RCV;
        } else if(byte != FLAG){
            state = START;
        }
        break;

    case A_RCV:
        if(byte == C_SET){
            state = C_RCV;
        } else if(byte != FLAG){
            state = START;
        }
        break;

    default:
        break;
    }
}

int c_check(unsigned char byte){
    switch (byte){
        case C_SET:
            control = SET;
            break;

        case C_UA:
            control = UA;
            break;

        case C_RR0:
            control = RR0;
            break;

        case C_RR1:
            control = RR1;
            break;

        case C_REJ0:
            control = REJ0;
            break;

        case C_REJ1:
            control = REJ1;
            break;

        case C_DISC:
            control = DISC;
            break;

        default:
            return 0;
            break;
        }
    return 1;
}
