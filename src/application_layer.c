// Application layer protocol implementation

#include "application_layer.h"
#include "application_layer_aux.h"
#include <stdio.h>
#include "link_layer.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

LinkLayer connectionParameters;
bool debug_application_layer = true;

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
        printf("Connection established\n\n");
    }

    //transferir/receber o ficheiro
    if(connectionParameters.role == LlTx){
        // eviar o ficheiro
        if(sendFile(filename) == -1){
            perror("Error sending file\n");
            return;
        }
    }
    else{
        // receber o ficheiro
        if(receiveFile() == -1){
            perror("Error receiving file\n");
            return;
        }
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

    printf("Writting\n");

    /*
    if(debug_application_layer){
        debug_print_frame(control_frame_start, control_frame_size);
    }
    */

    if(llwrite(control_frame_start, control_frame_size) == -1){
        perror("Error sending control frame\n");
        return -1;
    }

    return 1;
}

int receiveFile(){
    //receber o control frame para começar

    unsigned char* control_frame = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
    bool flag = true;
    int res;

    while(flag){
        res = llread(control_frame);
        if(res < 0){
            //lidar com os erros
            perror("Error receiving control frame\n");
            return -1;
        }
        else{
            //receber o control frame
            flag = false;
        }
    }
    
    unsigned long int file_size = 0;
    int filename_size = 0;

    if(debug_application_layer){
        debug_print_frame(control_frame, res);
    }

    unsigned char* filename = receiveControlPacket(control_frame, &file_size, &filename_size);

    //FILE* my_file = fopen((char *) filename, "wb+");
    
    /*
    if(debug_application_layer){
        debug_control_frame_rx(filename, strlen((char *) filename), file_size);
    }
    */

    free(filename);
    //receber o ficheiro

    //receber o cntrol frame para terminar a conecção
    return 1;
}

unsigned char* create_control_frame(char c, const char* filename, long int file_size, unsigned int* final_control_size){
    unsigned int pos = 0;
    
    int L1 = calculate_octets(file_size);

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

    if(debug_application_layer){
        debug_control_frame_tx(filename, L2, L1);
    }

    return control_packet;
}

unsigned char* receiveControlPacket(unsigned char* control_frame, unsigned long int* file_size, int* filename_size){

    //File Size
    unsigned char L1 = control_frame[2];
    unsigned char file_size_bytes[L1];

    for (int i = 0; i < L1; i++) {
        file_size_bytes[i] = control_frame[3 + i];
    }

    for(int i = 0; i < L1; i++){
        *file_size += file_size_bytes[i] << (8 * (L1 - i - 1));
    }

    //Filename
    unsigned char L2 = control_frame[4 + L1];
    unsigned char* filename = (unsigned char*)malloc(L2);

    for(int i = 0; i < L2; i++){
        filename[i] = control_frame[5 + L1 + i];
    }

    *filename_size = L2;

    return filename;
}

void debug_control_frame_tx(const char* filename, unsigned int filename_size, int size_of_file){
    if(filename == NULL){
        printf("Filename is NULL\n");
        return;
    }
    
    printf("\nFilename: ");
    for(int i = 0; i < filename_size; i++){
        printf("%c", filename[i]);
    }
    printf("\nFilename size: %d\n", filename_size);
    printf("Size of file: %d\n\n", size_of_file);

}


void debug_control_frame_rx(unsigned char* filename, unsigned int filename_size, int size_of_file){
    if(filename == NULL){
        printf("Filename is NULL\n");
        return;
    }
    
    printf("\nFilename: ");
    for(int i = 0; i < filename_size; i++){
        printf("%c", filename[i]);
    }
    printf("\nFilename size: %d\n", filename_size);
    printf("Size of file: %d\n\n", size_of_file);
}

int calculate_octets(long int file_size){
    if((file_size >> 24) > 0){
        return 4;
    }
    else if((file_size >> 16) > 0){
        return 3;
    }
    else if((file_size >> 8) > 0){
        return 2;
    }
        
    return 1; 
}

void debug_print_frame(unsigned char* frame, int frame_size){
    printf("\nFrame: ");
    for(int i = 0; i < frame_size; i++){
        printf("0x%02X ", frame[i]);
    }
    printf("\n\n");
}

