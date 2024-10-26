#ifndef APPLICATION_LAYER_AUX
#define APPLICATION_LAYER_AUX

#define DataSizeL1(n)   (n >> 8) & 0xFF
#define DataSizeL2(n)   n & 0xFF
#define DATA_SIZE(n,m)  ((n << 8) | m)


int sendFile(const char *filename);
int receiveFile(const char *filename);
unsigned char* create_control_frame(unsigned char c, const char* filename, long int file_size, unsigned int* final_control_size);
unsigned char* receiveControlPacket(unsigned char* control_frame, unsigned long int* file_size, int* filename_size);
void debug_control_frame_rx(unsigned char* filename, unsigned int filename_size, int size_of_file);
void debug_control_frame_tx(const char* filename, unsigned int filename_size, int size_of_file);
unsigned char calculate_octets(long int file_size);
int sendFileContent(unsigned char* file_content, long int file_size);
unsigned char* create_data_frame_packet(unsigned char* data_frame, int data_frame_size, int* packet_size, unsigned char sequence_number);
int receive_data_frame_packet(unsigned char* data_frame_packet, int data_frame_packet_size, unsigned char* data);
void debug_print_frame(unsigned char* frame, int frame_size);

#endif