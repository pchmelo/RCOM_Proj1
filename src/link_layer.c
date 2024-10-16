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
        if(!llopenRx){
            perror("Error llopenRx");
            return -1;
        }

    } else {
        // Send SET (write)
        // Wait UA (read)
        if(!llopenTx){
           perror("Error llopenTx");
           return -1; 
        }
    }

    
    return 1;
}


//temos que depois dar set a um alarm e lidar com isso
int llopenRx(){ 
    control = NOTHING_C;

    while(control != SET){
        if(!read_aux()){
            perror("Error reading SET");
            return -1;
        }
    }

    control = NOTHING_C;

    write_aux(ua_menssage, 5);

    return 1;
}

//temos que depois dar set a um alarm e lidar com isso
int llopenTx(){
    write_aux(set_menssage, 5); // Send SET
    
    control = NOTHING_C;

    while(control != UA){
        if(!read_aux()){
            perror("Error reading UA");
            return -1;
        }
    }

    control = NOTHING_C;

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

void stateMachineHandler(unsigned char byte){
    switch (state){
    case START:
        if(byte == FLAG){
            state = FLAG_RCV;
            buf[0] = byte;
        }
        break;
    
    case FLAG_RCV:
        if(byte == A){
            state = A_RCV;
            buf[1] = byte;
        } else if(byte != FLAG){
            state = START;
        }
        break;

    case A_RCV:
        if(c_check(byte)){
            state = C_RCV;
            buf[2] = byte;
        } else if(byte == FLAG){
            state = FLAG_RCV;
        }
        state = START;
        break;

    case C_RCV:
        if(byte == (buf[1] ^ buf[2])){
            state = BCC_OK;
            buf[3] = byte;
        } else if(byte == FLAG){
            state = FLAG_RCV;
        } else {
            state = START;
        }
        break;

    case BCC_OK:
        if(byte == FLAG){
            state = STOP;
            buf[4] = byte;
        } else {
            state = START;
        }
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

int read_aux(){
    state = START;
    unsigned char byte;

    while(state != STOP){
        if(readByteSerialPort(byte)){
            stateMachineHandler(byte);
        }
    }

    state = END;

    return 1;
}

int write_aux(unsigned char mensage, int numBytes){
    if(writeBytesSerialPort(mensage, numBytes) == -1){
        perror("Error sending SET");
        return -1;
    }

    return 1;
}
