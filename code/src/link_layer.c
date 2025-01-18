// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/time.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

struct timeval start, end;

struct timeval start, end;

//Global Variables

bool debug = false;
bool debug = false;

State state = END;
C_TYPE control = NOTHING_C;
bool frame_num = 0;
bool my_role;
bool my_role;

int alarmCounter = 0;
int timeout = 0;
int restransmissions = 0;
bool alarmFlag = false;

int num_errors = 0;

unsigned char buf[2*MAX_PAYLOAD_SIZE] = {0};
long int bytes_sent = 0;
long int bytes_received = 0;

//active debug


void alarmHandler(int signal){
    alarmFlag = false;
    alarmFlag = false;
    alarmCounter++;
    printf("Alarmcounter: %d\n", alarmCounter);
    printf("Alarmcounter: %d\n", alarmCounter);
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    gettimeofday(&start, NULL);
    gettimeofday(&start, NULL);
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0){
        return -1;
    }

    timeout = connectionParameters.timeout;
    restransmissions = connectionParameters.nRetransmissions;

    // TODO
    if(connectionParameters.role == LlRx){
        // Wait for a SET then ...
        // Send UA
        my_role = 0;
        my_role = 0;
        if(!llopenRx()){
            perror("Error llopenRx");
            return -1;
        }

    } else {
        // Send SET (write)
        // Wait UA (read)
        my_role = 1;
        my_role = 1;
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
        readBuf = read_aux(&numBytes, false);

        if(readBuf == NULL){
            perror("Error reading UA");
            free(readBuf);
            return -1;
        }
        if(readBuf != NULL){
            free(readBuf);
        } 
        if(readBuf != NULL){
            free(readBuf);
        } 
    }

    control = NOTHING_C;
    
    

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
    alarmCounter = 0;


    while(alarmCounter < tries && control != UA){
        alarmFlag = true;
        alarm(timeout);
        write_aux(set_menssage, 5); // Send SET
        readBuf = read_aux(&numBytes, true);

        if(readBuf != NULL){
            if(control == UA){
                break;
            }
            else{
                alarmCounter = 0;
                num_errors++;
            }
        }
        if(readBuf != NULL){
        if(readBuf != NULL){
            if(control == UA){
                break;
            }
            else{
                alarmCounter = 0;
                num_errors++;
            }
        }
        if(readBuf != NULL){
            free(readBuf);
        }
    }
    

    
    

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
    int localBufSize = 0;
    
    unsigned char bcc2 = calculate_BCC2(buf, bufSize);
    unsigned char *newBuf;

    unsigned char *buf_bcc2 = (unsigned char *)malloc(bufSize + 1 * sizeof(unsigned char));
    memcpy(buf_bcc2, buf, bufSize * sizeof(unsigned char));
    buf_bcc2[bufSize] = bcc2;

    newBuf = suffing_encode(buf_bcc2, bufSize + 1, &localBufSize);
    free(buf_bcc2);
    free(buf_bcc2);

    if(newBuf == NULL){
        perror("Error encoding suffing");
        return -1;
    }

    unsigned char frame[localBufSize + 6];

    int frameSize = mount_frame_menssage(localBufSize, newBuf, frame);
    free(newBuf);

    //colocar alarme
    bool flag = true;
    int tries = restransmissions;

    int numBytes;
    unsigned char* readBuf;

    
    alarmCounter = 0;
    bytes_sent += frameSize;

    printf("Frame number write: %d\n", frame_num);

    while(alarmCounter < tries && flag){
        alarmFlag = true;
        alarm(timeout);

        if(!write_aux(frame, frameSize)){
            perror("Error writtin mensage");
            return -1;
        }

        control = NOTHING_C;
        readBuf = read_aux(&numBytes, true);

        if(readBuf != NULL){
            int result = handle_llwrite_reception();    
        if(readBuf != NULL){
            int result = handle_llwrite_reception();    

            //mensagem recebida com sucesso
            if(result == 1){
                flag = false;
            }else{
                num_errors++;
                alarmCounter = 0;
            }
        }
        if(readBuf != NULL){
            free(readBuf);
        }        
    }
    
            if(result == 1){
                flag = false;
            }else{
                num_errors++;
                alarmCounter = 0;
            }
        }
        if(readBuf != NULL){
            free(readBuf);
        }        
    }
    
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
    bool flag = true;
    unsigned char *newBuf;
    int newBufSize;
    int result;
    bool flag = true;
    unsigned char *newBuf;
    int newBufSize;
    int result;

    while(flag){
        readBuf = read_aux(&numBytes, false);

        if (readBuf == NULL){
            perror("Error read_aux function");
            free(readBuf);
            return -1; 
        }
        
        bytes_sent += newBufSize;

        //destuffing
        newBuf = stuffing_decode(readBuf, numBytes, &newBufSize);
        //destuffing
        newBuf = stuffing_decode(readBuf, numBytes, &newBufSize);

        //verificar se há erros
        result = handle_llread_reception(newBuf, newBufSize);
        
        if(newBuf != NULL){
            free(readBuf);
        }
        control = NOTHING_C;
        //verificar se há erros
        result = handle_llread_reception(newBuf, newBufSize);
        
        if(newBuf != NULL){
            free(readBuf);
        }
        control = NOTHING_C;

        //mensagem recebida com sucesso
        if(result == 1){
            newBuf[newBufSize - 1] = '\0';
            frame_num = !frame_num;
        //mensagem recebida com sucesso
        if(result == 1){
            newBuf[newBufSize - 1] = '\0';
            frame_num = !frame_num;

            *packet = (unsigned char*)malloc((newBufSize - 1) * sizeof(unsigned char));
            memcpy(packet, newBuf, (newBufSize - 1) * sizeof(unsigned char));
            free(newBuf);
            bytes_received += numBytes;
            

            return newBufSize - 1;
        }

        

        num_errors++;
       
    }

    free(newBuf);
    printf("Error reading frame\n");
    printf("Error reading frame\n");
    
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


    if(my_role == 0){
        while(flag){

            control = NOTHING_C;
            readBuf = read_aux(&numBytes, false);

            if(control == DISC){
                //enviar UA
                write_aux(disc_menssage, 5);
                flag = false;
            }
            
            if(readBuf != NULL){
                free(readBuf);
            }
        }


        flag = true;
      

        while(flag){
            //read UA

            control = NOTHING_C;
            readBuf = read_aux(&numBytes, false);
            
            if(readBuf != NULL){
                alarmCounter = 0;
                if(control == DISC){
                //enviar DISC
                write_aux(disc_menssage, 5);
                } else if(control == UA){ 
                    flag = false;
                }
            }
            if(readBuf != NULL){
                free(readBuf);
            }
        }
    }
    //enviar DISC, receber DISC e enviar UA
    else{
        while (alarmCounter < tries && flag){
        while (alarmCounter < tries && flag){
            //eniando DISC
            alarmFlag = true;
            alarmFlag = true;
            write_aux(disc_menssage, 5);
            
            //receber DISC
            control = NOTHING_C;
            readBuf = read_aux(&numBytes, true);

            if(readBuf != NULL){
                alarmCounter = 0;
                if(control == DISC){
                    //enviar UA
                    write_aux(ua_menssage_tx, 5);
                    flag = false;

                } 
            }
            if(readBuf != NULL){
                free(readBuf);
            }
        }
        
        control = NOTHING_C;
        
    }

    if(showStatistics){
        display_statistics();
    if(showStatistics){
        display_statistics();
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
                state = BCC_OK;
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
        case BCC_OK:
            if(byte == FLAG){
                state = STOP_SMALL;
                buf[*pos] = byte;
                *pos += 1;
            } else {
                state = DATA;
                buf[*pos] = byte;
                *pos += 1;
            }
            
            break;

        case DATA:
            if(byte == FLAG){
                state = STOP_BIG;
                buf[*pos] = byte;
                *pos += 1;
            } else {
                state = DATA;
                buf[*pos] = byte;
                *pos += 1;
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
        while(state != STOP_BIG && state != STOP_SMALL && alarmFlag){
        while(state != STOP_BIG && state != STOP_SMALL && alarmFlag){
            if(readByteSerialPort(&byte)){
                stateMachineHandler(byte, &pos);
            }
        }

        if(!alarmFlag){
            return NULL;
        }


        if(!alarmFlag){
            return NULL;
        }

    }
    else{


        while(state != STOP_BIG && state != STOP_SMALL){
            if(readByteSerialPort(&byte)){
                stateMachineHandler(byte, &pos);
            }
        }
    }



    unsigned char *mensseBuf = (unsigned char *)malloc(pos * sizeof(unsigned char));
    memcpy(mensseBuf, buf, pos * sizeof(unsigned char));
    *readenBytes = pos;

    final_check();





    state = END;

    /*
    /*
    if(debug){
        debug_read(mensseBuf, *readenBytes);
    }
    */
    */

    return mensseBuf;
}

int write_aux(unsigned char* mensage, int numBytes){

    if(writeBytesSerialPort(mensage, numBytes) == -1){
        perror("Error sending SET");
        return -1;
    }

    return 1;
}

//0x7e -> 0x7d 0x5e
//0x7d -> 0x7d 0x5d
unsigned char* suffing_encode(const unsigned char *buf, int bufSize, int *newBufSize){
    
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
//remover duas flags, a, c e bcc1
unsigned char* stuffing_decode(unsigned char *buf, int bufSize, int *newBufSize){
    int maxBufSize = bufSize;
    unsigned char *newBuf = (unsigned char *)malloc((maxBufSize - 5) * sizeof(unsigned char));

    if(newBuf == NULL){
        perror("Error allocating memory");
        return NULL;bool get_frame_num(void);
    }

    int j = 0;

    for(int i = 4; i < bufSize - 1; i++){
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

//calculate BCC2 = D1 xor D2 xor ...bool get_frame_num(void); xor Dn
unsigned char calculate_BCC2(const unsigned char *buf, int bufSize){
    unsigned char bcc2 = 0x00;
    for(int i = 0; i < bufSize; i++){
        bcc2 ^= buf[i];
    }

    return bcc2;
}

//verify BCC2 == D1 xor D2 xor ... xor Dn
bool verify_BCC2(unsigned char *buf, int bufSize){
    unsigned char bcc2_calc = calculate_BCC2(buf, bufSize - 1);
    return buf[bufSize-1] == bcc2_calc;
}

int mount_frame_menssage(int numBytesMenssage, unsigned char *buf, unsigned char *frame){
    frame[0] = FLAG;
    frame[1] = A_Tx;

    if(!frame_num){
        frame[2] = C_FRAME0;
    } else {
        frame[2] = C_FRAME1;
    }

    frame[3] = A_Tx ^ frame[2];
    for(int i = 0; i < numBytesMenssage; i++){
        frame[i+4] = buf[i];
    }

    frame[numBytesMenssage + 4] = FLAG;
    return numBytesMenssage + 5;
}




int handle_llwrite_reception(){
    //frame_0 recebido com sucesso envia o frame_1 ou o contrário

    if((frame_num == 0 && control == RR1) || (frame_num == 1 && control == RR0)){
        frame_num = !frame_num;
        printf("Frame accepted\n");
        return 1;
    }
    //frame_0 foi recebido, mas com erros ou o contrário
    else if((frame_num == 0 && control == REJ0) || (frame_num == 1 && control == REJ1)){
        return 0;
    }

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
    if(control == SET){
        //enviar UA
        printf("Sending UA\n");
        write_aux(ua_menssage_tx, 5);
        return -2;
    }
    else if(control == DISC){
        //enviar DISC
        printf("Sending DISC\n");
        write_aux(disc_menssage, 5);
        return -3;
    }

    //verifica o BBC2
    if(!verify_BCC2(buf, bufSize)){
        //enviar REJ
        if(frame_num){
        if(frame_num){
            //enviar REJ0
            write_aux(rej1_menssage, 5);
        } else {
            //enviar REJ1
            write_aux(rej0_menssage, 5);
        }
        return -1;
    }

    //verifica se é frame_0 ou frame_1

    if(control == FRAME0){
        //enviar RR1
        printf("Sending RR1\n");
        write_aux(rr1_menssage, 5);
        if(frame_num == 0){
            return 1;
        } else {
            return -4;
        }
    } else if(control == FRAME1){
        //enviar RR0
        printf("Sending RR0\n");
        write_aux(rr0_menssage, 5);
        if(frame_num == 1){
            return 1;
        } else {
            return -4;
        }
    }
    return -5;
}



void debug_write(unsigned char *mensage, int numBytes){
    
    
    printf("\n%d bytes written\n", numBytes);
    
    printf("Sent: ");
    for(int i = 0; i < numBytes; i++){
        printf("0x%02X ", mensage[i]);
    }
    printf("\n");

}

void final_check(){
    if(state == STOP_BIG){
        if(control != FRAME1 && control != FRAME0){
            control = ERROR;
        }
    }
}


void display_statistics(){
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("\n--------------Statistics--------------\n");
    printf("Number of Frames with errors: %d\n", num_errors);
    printf("Time taken: %0.2f seconds\n", time_taken);
    
    if(my_role == 0){
        printf("Number of bytes received: %ld\n", bytes_received);
        printf("Total file size: %ld\n", bytes_sent);
    } else {
        printf("Number of bytes sent: %ld\n", bytes_sent);
    }
   

}
