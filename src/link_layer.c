// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

//Global Variables
#define BufferSize 255
unsigned char buf[BufferSize] = {0};

State state = END;
C_TYPE control = NOTHING_C;
bool frame_num = 0;

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

//vai montar a frame neste formato: FLAG A C BCC1 D1 D2 ... Dn BCC2 FLAG
// A -> 0x03
// C -> 0x00 ou 0x80 (dependendo do frame)
// calcular BCC2
// byte stuffing
// montar e enviar a frame
// esperar por RR ou REJ

int llwrite(const unsigned char *buf, int bufSize){
    int bufSize = 0;
    
    unsigned char bcc2 = calculate_BCC2(buf, bufSize);
    unsigned char *newBuf;

    newBuf = suffing_encode(buf, bufSize, &bufSize);

    if(newBuf == NULL){
        perror("Error encoding suffing");
        return 1;
    }

    unsigned char frame[bufSize + 6];

    int frameSize = mount_frame_menssage(bufSize, newBuf, frame, bcc2);
    free(newBuf);

    //colocar alarme
    bool flag = true;

    while(flag){
        if(!write_aux(frame, frameSize)){
            perror("Error writtin mensage");
            return 1;
        }

        control = NOTHING_C;

        if(!read_aux()){
            perror("Error read_aux function");
            return 1; 
        }

        
    }

    

    

    return 0;
}

//receber frame
//byte destuffing
//verificar BCC2
//enviar RR ou REJ dependendo da frame recebida

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
        
        case C_FRAME0:
            control = FRAME0;
            break;
        
        case C_FRAME1:
            control = FRAME1;
            break;

        default:
            return 0;
            break;
        }
    return 1;
}

//adicionar alarme
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

//0x7e -> 0x7d 0x5e
//0x7d -> 0x7d 0x5d
unsigned char* suffing_encode(unsigned char *buf, int bufSize, int *newBufSize){
    
    int maxBufSize = 2 * bufSize;
    unsigned char *newBuf = (unsigned char *)malloc(maxBufSize * sizeof(unsigned char));

    if(newBuf == NULL){
        perror("Error allocating memory");
        return NULL;
    }

    int j = 0;

    for(int i = 0; i < bufSize; i++){
        if(buf[i] == 0x7e){
            newBuf[j] = 0x7d;
            j++;
            newBuf[j] = 0x5e;
            j++;
        } else if(buf[i] == 0x7d){
            newBuf[j] = 0x7d;
            j++;
            newBuf[j] = 0x5d;
            j++;
        } else {
            newBuf[j] = buf[i];
            j++;
        }
    }

    *newBufSize = j;
    newBuf = (unsigned char *)realloc(newBuf, j * sizeof(unsigned char));
    if(newBuf == NULL){
        perror("Error reallocating memory");
        return NULL;
    }

    return newBuf;
}

//0x7d 0x5e -> 0x7e
//0x7d 0x5d -> 0x7d
unsigned char* stuffing_decode(unsigned char *buf, int bufSize, int *newBufSize){
    int maxBufSize = bufSize;
    unsigned char *newBuf = (unsigned char *)malloc(maxBufSize * sizeof(unsigned char));

    if(newBuf == NULL){
        perror("Error allocating memory");
        return NULL;
    }

    int j = 0;

    for(int i = 0; i < bufSize; i++){
        if(buf[i] == 0x7d && buf[i+1] == 0x5e){
            newBuf[j] = 0x7e;
            j++;
            i++;
        } else if(buf[i] == 0x7d && buf[i+1] == 0x5d){
            newBuf[j] = 0x7d;
            j++;
            i++;
        } else {
            newBuf[j] = buf[i];
            j++;
            i++;
        }
    }

    *newBufSize = j;
    newBuf = (unsigned char *)realloc(newBuf, j * sizeof(unsigned char));
    if(newBuf == NULL){
        perror("Error reallocating memory");
        return NULL;
    }

    return newBuf;
}

//calculate BCC2 = D1 xor D2 xor ... xor Dn
char calculate_BCC2(unsigned char *buf, int bufSize){
    char bcc2 = 0x00;
    for(int i = 0; i < bufSize; i++){
        bcc2 ^= buf[i];
    }
    return bcc2;
}

//verify BCC2 == D1 xor D2 xor ... xor Dn
bool verify_BCC2(unsigned char *buf, int bufSize, char bcc2){
    char bcc2_calc = calculateBCC2(buf, bufSize);
    return bcc2 == bcc2_calc;
}

int mount_frame_menssage(int numBytesMenssage, unsigned char *buf, unsigned char *frame, unsigned char bb2){
    frame[0] = FLAG;
    frame[1] = A;

    if(frame_num == FRAME0){
        frame[2] = C_FRAME0;
    } else {
        frame[2] = C_FRAME1;
    }

    frame[3] = A ^ control;
    for(int i = 0; i < numBytesMenssage; i++){
        frame[i+4] = buf[i];
    }

    frame[numBytesMenssage + 4] = bb2;
    frame[numBytesMenssage + 5] = FLAG;

    return numBytesMenssage + 6;
}




int handle_llwrite_reception(){
    //frame_0 recebido com sucesso envia o frame_1 ou o contrário
    if((frame_num == 0 && control == RR1) || (frame_num == FRAME1 && control == RR0)){
        !frame_num;
        return 1;
    }
    //frame_0 foi recebido, mas com erros ou o contrário
    else if((frame_num == 0 && control == RR0) || (frame_num == FRAME1 && control == RR1)){
        return 2;
    }

    //lidar com os restos dos casos aqui

    return 0;
}

