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
        if(receiveFile(filename) == -1){
            perror("Error receiving file\n");
            return;
        }
    }

    //fechar a conecção
    llclose(1);

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

    printf("Writting the control word Start\n");

    /*
    if(debug_application_layer){
        debug_print_frame(control_frame_start, control_frame_size);
    }
    */

    if(llwrite(control_frame_start, control_frame_size) == -1){
        perror("Error sending control frame\n");
        return -1;
    }

    free(control_frame_start);

    //ler conteudo do ficheiro para um buffer
    unsigned char* file_content = (unsigned char*)malloc(sizeof(unsigned char) * my_file_size);
    fread(file_content, sizeof(unsigned char), my_file_size, my_file);

    printf("Writting the file content\n\n");

    //enviar o ficheiro
    if(sendFileContent(file_content, my_file_size) == -1){
        perror("Error sending file content\n");
        return -1;
    }   
    free(file_content);

    printf("Writting the control frame end\n\n");

    //enviar o control frame para terminar a conecção
    unsigned char* control_frame_end = create_control_frame(3, filename, my_file_size, &control_frame_size);
    if(llwrite(control_frame_end, control_frame_size) == -1){
        perror("Error sending control frame\n");
        return -1;
    }
    free(control_frame_end);

    return 1;
}

int receiveFile(const char *filename){
    //receber o control frame para começar

    unsigned char* control_frame = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
    bool flag = true;
    int res;

    printf("Reading the control word start\n\n");

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

    /*
    if(debug_application_layer){
        debug_print_frame(control_frame, res);
    }
    */

    unsigned char* my_filename = receiveControlPacket(control_frame, &file_size, &filename_size);

    if(debug_application_layer){
        debug_control_frame_rx(my_filename, filename_size, file_size);
    }

    FILE* my_file = fopen((char *) filename, "wb+");
    free(my_filename);

    unsigned char* data_frame_packet;
    unsigned char* data_frame;
    flag = true;
    int data_size = 0;
    int frame_num = 0;

    printf("Reading the file content\n\n");

    while(flag){
        data_frame_packet = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
        res = llread(data_frame_packet);
        if(res < 0){
            //lidar com os erros
            perror("Error receiving data frame\n");
        }
        else{
            //receber o data frame

            
            if(debug_application_layer){
                debug_print_frame(data_frame_packet, res);
            }
            
            
            data_frame = receive_data_frame_packet(data_frame_packet, res, &data_size);

            
            if(debug_application_layer){
                debug_print_frame(data_frame, data_size);
            }
            

            if(data_frame == NULL){
                printf("Reading the control word end\n\n");
                flag = false;
            }
            else{
                printf("Frame number: %d\n", frame_num);
                frame_num++;
                fwrite(data_frame, sizeof(unsigned char), data_size, my_file);
                free(data_frame_packet);
                free(data_frame);
            }
        
        }
    }

    free(data_frame_packet);
    free(data_frame);

    fclose(my_file);
    
    
    return 1;
}

unsigned char* create_control_frame(unsigned char c, const char* filename, long int file_size, unsigned int* final_control_size){
    unsigned int pos = 0;
    
    unsigned char L1 = calculate_octets(file_size);

    unsigned char L2 = strlen(filename);
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

unsigned char calculate_octets(long int file_size) {
    unsigned char octets = 0;
    while (file_size > 0) {
        file_size >>= 8; // Shift right by 8 bits (1 byte)
        octets++;
    }
    return octets;
}


int sendFileContent(unsigned char* file_content, long int file_size){
    long int bytes_sent = 0;
    int sequence_number = 0;
    unsigned char* data_frame;
    unsigned char* data_frame_packet;

    while(bytes_sent < file_size){
        int data_frame_size = (file_size - bytes_sent) > (MAX_PAYLOAD_SIZE - 4) ? (MAX_PAYLOAD_SIZE - 4) : (file_size - bytes_sent);
        
        data_frame = (unsigned char*)malloc(data_frame_size);
        memcpy(data_frame, file_content + bytes_sent, data_frame_size);

        int packet_size = 0;
        data_frame_packet = create_data_frame_packet(data_frame, data_frame_size, &packet_size, sequence_number);


        if(llwrite(data_frame_packet, packet_size) == -1){
            perror("Error sending file content\n");
            return -1;
        }

        /*
        if(debug_application_layer){
            debug_print_frame(data_frame_packet, packet_size);
        }
        */

        free(data_frame);
        free(data_frame_packet);

        bytes_sent += data_frame_size;
        sequence_number ++;
    }

    return 1;
}   

unsigned char* create_data_frame_packet(unsigned char* data_frame, int data_frame_size, int* packet_size, unsigned char sequence_number){
    *packet_size = data_frame_size + 4;
    unsigned char* packet = (unsigned char*)malloc(*packet_size);

    packet[0] = 2;
    packet[1] = sequence_number % 100;
    packet[2] = DataSizeL1(data_frame_size);
    packet[3] = DataSizeL2(data_frame_size);

    memcpy(packet + 4, data_frame, data_frame_size);
    
    return packet;
}

unsigned char* receive_data_frame_packet(unsigned char* data_frame_packet, int data_frame_packet_size, int* data_size){
    if(data_frame_packet[0] != 2){
        return NULL;
    }

    unsigned char L1 = data_frame_packet[2];
    unsigned char L2 = data_frame_packet[3];

    *data_size = DATA_SIZE(L1, L2);

    unsigned char* data = (unsigned char*)malloc(*data_size);

    memcpy(data, data_frame_packet + 4, *data_size);

    return data;
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

void debug_print_frame(unsigned char* frame, int frame_size){
    if(frame == NULL){
        printf("Frame is NULL\n");
        return;
    }
    
    printf("\nFrame: ");
    for(int i = 0; i < frame_size; i++){
        printf("0x%02X ", frame[i]);
    }

    printf("\nFrame size: %d\n", frame_size);

    printf("\n\n");
}

