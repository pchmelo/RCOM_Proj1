// Application layer protocol implementation

#include "application_layer.h"

#include <stdio.h>
#include "link_layer.h"
#include <string.h>

LinkLayerRole role;
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

    if (llopen(connectionParameters) < 0) {
        perror("Error llopen");
        return;
    }
    else{
        printf("Connection established\n");
    }



}
