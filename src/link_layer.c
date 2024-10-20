// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"
#include <signal.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

//Global Variables
#define BufferSize 255
unsigned char buf[BufferSize] = {0};

State state = END;
C_TYPE control = NOTHING_C;
bool frame_num = 0;
LinkLayerRole role;

int alarmCounter = 0;
int timeout = 0;
int restransmissions = 0;
bool alarmFlag = false;

void alarmHandler(int signal){
    alarmFlag = true;
    alarmCounter++;
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0){
        return -1;
    }

    role = connectionParameters.role;
    timeout = connectionParameters.timeout;
    restransmissions = connectionParameters.nRetransmissions;

    // TODO
    if(connectionParameters.role == LlRx){
        // Wait for a SET then ...
        // Send UA
        if(!llopenRx()){
            perror("Error llopenRx");
            return -1;
        }

    } else {
        // Send SET (write)
        // Wait UA (read)
        if(!llopenTx()){
           perror("Error llopenTx");
           return -1; 
        }
    }

    
    return 1;
}


//temos que depois dar set a um alarm e lidar com isso
int llopenRx(){ 

    
    control = NOTHING_C;
    unsigned char *readBuf;
    int numBytes;

    while(control != SET){
        readBuf = read_aux(numBytes, false);

        if(readBuf == NULL){
            perror("Error reading UA");
            free(readBuf);
            return -1;
        }

    }

    control = NOTHING_C;
    free(readBuf);

    write_aux(ua_menssage_tx, 5);
    
    return 1;
}

//temos que depois dar set a um alarm e lidar com isso
//return 1 -> UA
//return 0 -> Tries expired
int llopenTx(){
    (void) signal(SIGALRM, alarmHandler);
    
    unsigned char *readBuf;
    int numBytes;
    int tries = restransmissions;
    control = NOTHING_C;

    while(tries > 0 || control != UA){
        write_aux(set_menssage, 5); // Send SET
        alarm(timeout);
        alarmFlag = false;

        
        readBuf = read_aux(numBytes, true);

        if(readBuf == NULL){
            perror("Error reading UA");
            free(readBuf);
            return -1;
        }
        tries--;
    }
    

    free(readBuf);

    return control == UA;
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
        return -1;
    }

    unsigned char frame[bufSize + 6];

    int frameSize = mount_frame_menssage(bufSize, newBuf, frame, bcc2);
    free(newBuf);

    //colocar alarme
    bool flag = true;
    int tries = restransmissions;

    int numBytes;
    unsigned char* readBuf;

    while(tries < 0 || flag){
        alarmFlag = false;
        alarm(timeout);

        if(!write_aux(frame, frameSize)){
            perror("Error writtin mensage");
            return -1;
        }

        control = NOTHING_C;
        readBuf = read_aux(numBytes, true);

        if(readBuf == NULL){
            perror("Error read_aux function");
            free(readBuf);
            return -1; 
        }

            //mensagem recebida com sucesso
        if(handle_llwrite_reception()){
            flag = false;
        }

        
        tries--;
    }

    free(readBuf);
    control = NOTHING_C;

    if(flag){
        return -1;
    }

    return bufSize;
}

//receber frame
//byte destuffing
//verificar BCC2
//enviar RR ou REJ dependendo da frame recebida
//return -2 -> SET
//return -3 -> DISC


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet){
    // TODO
    int numBytes;
    unsigned char *readBuf;
    control = NOTHING_C;

    readBuf = read_aux(numBytes, false);

    if (readBuf == NULL){
        perror("Error read_aux function");
        free(readBuf);
        return -1; 
    }

    unsigned char *newBuf;
    int newBufSize;

    //destuffing
    newBuf = stuffing_decode(readBuf, numBytes, &newBufSize);

    //verificar se há erros
    int result = handle_llread_reception(newBuf, newBufSize);
    free(readBuf);
    free(newBuf);
    control = NOTHING_C;

    //mensagem recebida com sucesso
    if(result == 1){
        packet = newBuf;
        return newBufSize;
    }
    
    //alguma inconsistência
    return result;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){

    unsigned char *readBuf;
    int numBytes;
    (void) signal(SIGALRM, alarmHandler);
    int tries = restransmissions;
    bool flag = true;

    //enviar DISC e receber UA, caso contrário reenviar DISC
    if(role == LlRx){
        while(tries > 0 || flag){
            //read UA

            alarm(timeout);
            alarmFlag = false;

            control = NOTHING_C;
            readBuf = read_aux(numBytes, true);

            if(control == DISC){
                //enviar DISC
                write_aux(disc_menssage_rx, 5);
            } else if(control == UA){ 
                flag = false;
            }
            
            tries--;
        }
    }
    //enviar DISC, receber DISC e enviar UA
    else{
        while (tries > 0 || flag){
            //eniando DISC
            write_aux(disc_menssage, 5);
            
            alarm(timeout);
            alarmFlag = false;
            
            //receber DISC
            control = NOTHING_C;
            readBuf = read_aux(numBytes, true);

            if(control == DISC){
                //enviar UA
                write_aux(ua_menssage_tx, 5);
                flag = false;
            } 
            
            tries--;
        }
        
        control = NOTHING_C;
        
    }

    free(readBuf);
    if(flag){
        return -1;
    }

    int clstat = closeSerialPort();
    return clstat;
}

void stateMachineHandler(unsigned char byte, int *pos){
    switch (state){
    case START:
        if(byte == FLAG){
            state = FLAG_RCV;
            buf[*pos] = byte;
            *pos += 1;
        }
        break;
    
    case FLAG_RCV:
        if(byte == A_Tx || byte == A_Rx){
            state = A_RCV;
            buf[*pos] = byte;
            *pos += 1;
        } else if(byte != FLAG){
            state = START;
            *pos = 0;
        }
        break;

    case A_RCV:
        if(c_check(byte)){
            state = C_RCV;
            buf[*pos] = byte;
            *pos += 1;
        } else if(byte == FLAG){
            state = FLAG_RCV;
            *pos = 1;
        }
        else{
            state = START;
            *pos = 0;
        }
        
        break;

    case C_RCV:
        if(byte == (buf[1] ^ buf[2])){
            state = DATA;
            buf[*pos] = byte;
            *pos += 1;
        } else if(byte == FLAG){
            state = FLAG_RCV;
            *pos = 1;
        } else {
            state = START;
            *pos = 0;
        }
        break;

    case DATA:
        if(byte == FLAG){
            state = STOP;
            buf[*pos] = byte;
            *pos += 1;
        } else {
            state = DATA;
            buf[*pos] = byte;
            *pos += 1;
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
unsigned char* read_aux(int *readenBytes, bool alarm){
    state = START;
    unsigned char byte;
    int pos = 0;
    if(alarm){
        while(state != STOP || alarmFlag == false){
            if(readByteSerialPort(byte)){
                stateMachineHandler(byte, pos);
            }
        }
    }
    else{
        while(state != STOP){
            if(readByteSerialPort(byte)){
                stateMachineHandler(byte, pos);
            }
        }
    }
    

    unsigned char *mensseBuf = (unsigned char *)realloc(buf, pos * sizeof(unsigned char));
    readenBytes = pos;

    state = END;

    return mensseBuf;
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
bool verify_BCC2(unsigned char *buf, int bufSize){
    char bcc2_calc = calculateBCC2(buf, bufSize);
    return buf[bufSize-2] == bcc2_calc;
}

int mount_frame_menssage(int numBytesMenssage, unsigned char *buf, unsigned char *frame, unsigned char bb2){
    frame[0] = FLAG;
    frame[1] = A_Tx;

    if(frame_num == FRAME0){
        frame[2] = C_FRAME0;
    } else {
        frame[2] = C_FRAME1;
    }

    frame[3] = A_Tx ^ control;
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
        return 0;
    }

    //lidar com os restos dos casos aqui

    return 0;
}

//return 1 -> RR
//return -1 -> REJ
//return 0 -> Same
//return -2 -> SET
//return -3 -> DISC
//return -4 -> Unknown

int handle_llread_reception(unsigned char *buf, int bufSize){
    
    //verifica se é SET ou DISC
    if(control == C_SET){
        //enviar UA
        write_aux(ua_menssage_tx, 5);
        return -2;
    }
    else if(control == C_DISC){
        //enviar DISC
        write_aux(disc_menssage, 5);
        return -3;
    }

    //verifica o BBC2
    if(!verify_BCC2(buf, bufSize)){
        //enviar REJ
        if(frame_num == 0){
            //enviar REJ0
            write_aux(rej0_menssage, 5);
        } else {
            //enviar REJ1
            write_aux(rej1_menssage, 5);
        }
        return -1;
    }

    //verifica se é frame_0 ou frame_1

    if(control == C_FRAME0){
        //enviar RR1
        write_aux(rr1_menssage, 5);
        if(frame_num == 0){
            return 1;
        } else {
            return 0;
        }
    } else if(control == C_FRAME1){
        //enviar RR0
        write_aux(rr0_menssage, 5);
        if(frame_num == 1){
            return 1;
        } else {
            return 0;
        }
    }
    return -4;
}
