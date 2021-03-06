#include<stdio.h>
#include<sys/socket.h>
#include <string.h> 
#include "ccitt16.h"
#include "stdint.h"

void secondary(int client_sock) {
    int read_size, numPackets;
	unsigned char msg[7];
	unsigned char data[4];
	unsigned char sequence_number = 0;
	numPackets = 0;


    // Receive a message from client
	// read_size - the length of the received message 
	// 				or an error code in case of failure
	// msg - a buffer where the received message will be stored
	// 149 - the size of the receiving buffer (any more data will be 
	// 		delievered in subsequent "recv" operations
	// 0 - Options and are not necessary here
    while( (read_size = recv(client_sock , msg , 6 , 0)) > 0 )
    {
		usleep(1000);
		// Null termination since we will use strlen(msg) later on
		msg[6] = '\0';

		data[0] = msg[0];
		data[1] = msg[1];
		data[2] = msg[2];
		data[3] = msg[3];
		int crc_check = 1;
		
		//Manually checking crc (check_crc) was not working
		short int crc = calculate_CCITT16(data, 2, GENERATE_CRC);
		unsigned char crc_upper = (uint8_t) (crc >> 8);
		unsigned char crc_lower = (uint8_t) (crc);
		//printf("CRC: %02X%02X\n\r",crc_upper,crc_lower);
		if(crc_upper == msg[4] && crc_lower == msg[5]){
			crc_check = 0;
		}

		//CRC valid and in order
		if(crc_check == 0 && ((unsigned char) msg[1]) == sequence_number){
			//increment sequence number
			sequence_number++;
			
			printf("Reported >> Type: %02d, SN: %02d, D1: %c, D2: %c, CRC: %02X%02X\n\r", msg[0], msg[1], msg[2], msg[3],msg[4], msg[5]);
			//print message and sequence number
			//constructing reply message
			unsigned char reply[6];

			//ACK value
			reply[0] = 2;
			//add updated sequence number
			reply[1] = sequence_number;
			//add arbitrary data to fill space
			reply[2] = 1;
			reply[3] = 1;
			
			reply[4] = '\0';
			
			//calculate crc
			short int crc = calculate_CCITT16(reply, 2, GENERATE_CRC);
			unsigned char crc_upper = (uint8_t) (crc >> 8);
			unsigned char crc_lower = (uint8_t) (crc);
			reply[4] = crc_upper;
			reply[5] = crc_lower;

        		//send the reply back to client
			numPackets = numPackets + 1;
			send(client_sock , reply , strlen(reply) , 0);
			printf("ACK      >> Type: %02d, SN: %02d, D1: %d, D2: %d, CRC: %02X%02X, Packet: %d\n\r", reply[0], reply[1], reply[2], reply[3],reply[4], reply[5],numPackets);
			
		}
		//CRC valid but not in order
		else if(crc_check == 0 && ((int) msg[1]) != sequence_number){
			//print sequence number
			printf("OOO      >> Type: %02d, SN: %02d, D1: %c, D2: %c, CRC: %02X%02X\n\r", msg[0], msg[1], msg[2], msg[3],msg[4], msg[5]);

			//constructing reply message
			unsigned char reply[6];
			
			//ACK value
			reply[0] = 3;
			//add old sequence number
			reply[1] = sequence_number;
			//add arbitrary data to fill space
			reply[2] = 1;
			reply[3] = 1;
			
			reply[4] = '\0';

			//calculate crc
			short int crc = calculate_CCITT16(reply, 2, GENERATE_CRC);
			unsigned char crc_upper = (uint8_t) (crc >> 8);
			unsigned char crc_lower = (uint8_t) (crc);
			reply[4] = crc_upper;
			reply[5] = crc_lower;
			numPackets = numPackets + 1;
			printf("NAK      >> Type: %02d, SN: %02d, Packet: %d\n\r", reply[0], reply[1],numPackets);

        		//send the reply back to client
			send(client_sock , reply , strlen(reply) , 0);
		}
		//CRC invalid, has error
		else if(crc_check != 0){
			//print sequence number
			printf("ERROR!   >> Type: %02d, SN: %02d\n\r", msg[0], msg[1]);

			char reply[6];
			//NAK value
			reply[0] = 3;
			//add old sequence number
			reply[1] = sequence_number;
			//add arbitrary data to fill space
			reply[2] = 1;
			reply[3] = 1;
			reply[4] = '\0';

			//calculate crc
			short int crc = calculate_CCITT16(reply, 2, GENERATE_CRC);
			unsigned char crc_upper = (uint8_t) (crc >> 8);
			unsigned char crc_lower = (uint8_t) (crc);
			reply[4] = crc_upper;
			reply[5] = crc_lower;

			numPackets = numPackets + 1;
			printf("NAK      >> Type: %02d, SN: %02d, Packet: %d\n\r", reply[0], reply[1],numPackets);

        		//send the reply back to client
			send(client_sock , reply , strlen(reply) , 0);
		}


    }
     
    // Might move back to receiver
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

}







