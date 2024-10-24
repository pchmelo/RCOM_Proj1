// Application layer protocol implementation

#include "application_layer.h"
#include <math.h>
#include <stdio.h>
#include "link_layer.h"
#include <string.h>
#include <stdbool.h>

LinkLayer connectionParameters;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){

    for (int i = 0; serialPort[i] != '\0'; i++) {
        strncat(connectionParameters.serialPort, &serialPort[i], 1);
    }
    
    connectionParameters.role = (strcmp(role, "tx") == 0) ? LlTx : LlRx; 
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    //estabelecer a conecção
    if (llopen(connectionParameters) < 0) {
        perror("Error llopen");
        return;
    }
    else{
        printf("Connection established\n");
    }

    //transferir/receber o ficheiro
    if(connectionParameters.role == LlTx){
        // eviar o ficheiro
        if(sendFile(filename) == -1){
            perror("Error sending file");
            return;
        }
    }
    else{
        // receber o ficheiro
    }

    //fechar a conecção


}

int sendFile(const char *filename){

    unsigned int control_frame_size = 0;
    FILE* my_file = fopen(filename, "rb");
    
    if(my_file == NULL){
        perror("Error opening file");
        return -1;
    }

    int pf = ftell(my_file);
    fseek(my_file, 0, SEEK_END);
    long int my_file_size = ftell(my_file) - pf;
    fseek(my_file, pf, SEEK_SET);

    unsigned char* control_frame_start = create_control_frame(1, filename, my_file_size, &control_frame_size);

    if(llwrite(control_frame_start, control_frame_size) == -1){
        perror("Error sending control frame");
        return -1;
    }

    return 1;
}

void receiveFile(){
    //receber o control frame para começar

    unsigned char* control_frame = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
    bool flag = true;
    int res;

    while(flag){
        res = llread(control_frame);
        if(res < 0){
            //lidar com os erros
            perror("Error receiving control frame");
            return;
        }
        else{
            //receber o control frame
            flag = false;
        }
    }
    
    unsigned long file_size = 0;
    unsigned char* filename = receiveControlPacket(control_frame, res, &file_size);

    //receber o ficheiro

    //receber o cntrol frame para terminar a conecção
}

unsigned char* create_control_frame(char c, const char* filename, long int file_size, unsigned int* final_control_size){
    unsigned int pos = 0;
    
    float octs = log2f ((float)file_size/8.0);
    int L1 = (int) ceil(octs);

    int L2 = strlen(filename);
    *final_control_size = 5 + L1 + L2; //c + T1 + L1 + file_size + T2 + L2 + file_name

    unsigned char *control_packet = (unsigned char*)malloc(*final_control_size);

    control_packet[pos] = c;
    pos++;

    control_packet[pos] = 0;
    pos++;

    control_packet[pos] = L1;
    pos++;

    //colocar em big endian
    for (int i = 0; i < L1; i++) {
        control_packet[pos] = (file_size >> (8 * (L1 - i - 1))) & 0xFF;
        pos++;
    }

    control_packet[pos] = 1;
    pos++;

    control_packet[pos] = L2;
    pos++;

    for (int i = 0; i < L2; i++) {
        control_packet[pos] = filename[i];
        pos++;
    }

    return control_packet;
}

unsigned char* receiveControlPacket(unsigned char* control_frame, int res, &file_size){

}


