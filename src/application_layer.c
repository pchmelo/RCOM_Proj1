// Application layer protocol implementation

#include "application_layer.h"

#include <stdio.h>
#include "link_layer.h"

LinkLayerRole role;
LinkLayer connectionParameters;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){
    
    role = (role == "tx") ? LlTx : LlRx;
    
    for (int i = 0; serialPort[i] != '\0'; i++) {
        strncat(connectionParameters.serialPort, &serialPort[i], 1);
    }
    
    connectionParameters.role = role;   
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if (llopen(connectionParameters) < 0) {
        perror("Error llopen");
        return;
    }

}
