#ifndef APPLICATION_LAYER_AUX
#define APPLICATION_LAYER_AUX


int sendFile(const char *filename);
int receiveFile();
unsigned char* create_control_frame(char c, const char* filename, long int file_size, unsigned int* final_control_size);
unsigned char* receiveControlPacket(unsigned char* control_frame, unsigned long int* file_size, int* filename_size);
void debug_control_frame_rx(unsigned char* filename, unsigned int filename_size, int size_of_file);
void debug_control_frame_tx(const char* filename, unsigned int filename_size, int size_of_file);
int calculate_octets(long int file_size);

#endif