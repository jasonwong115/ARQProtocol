#include <stdio.h> 
#include <string.h>    
#include <sys/socket.h>    
#include <stdlib.h>
#include "ccitt16.h"
#include "stdint.h"

//Size of send window
#define WINDOW 3

//Initialize global variables
//packet to be sent to receiver
unsigned char packet[6];
//Send window, contains SN if waiting for ack/nak or -99 otherwise
int window[WINDOW];
short int crc;
unsigned char crc_upper;
unsigned char crc_lower;
//total number of packets sent
int numPackets;

/*PROTOTYPES*/
void generatePacket(unsigned char cData[], unsigned char packetNumber, double ber);
void IntroduceError(char *data, double p);

//Loop to communicate with secondary receiver
void primary(int sockfd, double ber) {
    numPackets = 0;
    //Initialize all necessary variables
    int read_size, i, waiting, windowend;
    //msg is packet sent to receiver, srv_reply is packet from receiver, data is used to encapsulate data before crc added
    unsigned char msg[100], srv_reply[7], data[3], test_crc[4];
    char* alphabet = "abcdefghijklmnopqrstuvwxyz";
    int SNlast, SN, crc_check, packetType = 0;


    //send initial data (in this case is the sice of WINDOW which is 3 packets)
    for(i = 0; i < WINDOW; i++){
	data[0] = alphabet[(2 * i)];
	data[1] = alphabet[(2 * i) + 1];
	data[2] = '\0';
	generatePacket(data,i,ber);
	if( send(sockfd , packet, 6, 0) < 0)
		perror("Send failed");
	window[i] = i;
    }

    SNlast = 0;

    //keep communicating with server
    while(1)
    {
	//Check for any messages from receiver
	if( (read_size = recv(sockfd , srv_reply , 6 , 0)) < 0)
    		perror("recv failed");
	
	srv_reply[6] = '\0';

	//Assign received packet to new array for crc checking
	test_crc[0]=srv_reply[0];
	test_crc[1]=srv_reply[1];
	test_crc[2]=srv_reply[2];
	test_crc[3]=srv_reply[3];
	test_crc[4]='\0';

	crc_check = 0;

	//check nak/ack for error (for this assignment the receiver did not introduce any errors)
	int crc_check2 = calculate_CCITT16(test_crc, 2, GENERATE_CRC);
	unsigned char crc_upper2 = (uint8_t) (crc_check2 >> 8);
	unsigned char crc_lower2 = (uint8_t) (crc_check2);
	if(crc_upper2 == srv_reply[4] && crc_lower2 == srv_reply[5]){
		crc_check = 0;
		packetType = srv_reply[0];
		SN = srv_reply[1];
	}
	
	
	//Last packet was received!
	if(crc_check==0 && SN == 13 && packetType == 2){
		printf("Received ACK >> Type: %02d, SN: %02d, D1: %02d, D2: %02d, CRC: %02X%02X\n\r", srv_reply[0], srv_reply[1], srv_reply[2], srv_reply[3],srv_reply[4], srv_reply[5]);
		printf("All packets sent! Exiting...\n\r");
		exit(0);

	//CRC valid and ack received
	}else if(crc_check == 0 && packetType == 2){
		printf("Received ACK >> Type: %02d, SN: %02d, D1: %02d, D2: %02d, CRC: %02X%02X\n\r", srv_reply[0], srv_reply[1], srv_reply[2], srv_reply[3],srv_reply[4], srv_reply[5]);
		windowend = (SNlast + WINDOW - 1);

		//If SN in window, update window otherwise discard
		if(SN >= SNlast && SN <= windowend){
			int temp = 0;

			//Find position of SN in window
			for(i = 0 ; i < WINDOW ; i++){
				if(window[i]==(SN - 1)){
					temp = i;
					break;
				}
			}

			//shift window by position of SN in window
			for(i = 0; i < temp + 1; i++){
				int temp2 = window[2] + 1;
				window[0]=window[1];
				window[1]=window[2];
				window[2]=-99;
			}

			SNlast = SN;
			
			for(i = 0 ; i < WINDOW ; i++){
				if(window[i] < -10){
					//send new data if send window allows
					if(SNlast + i < 13){
						data[0] = alphabet[(2 * (SNlast+ i))];
						data[1] = alphabet[(2 * (SNlast+ i)) + 1];
						data[2] = '\0';
						//update window and send if possible
						generatePacket(data,SNlast+i,ber);
						if( send(sockfd , packet, 6, 0) < 0)
							perror("Send failed");
						window[i] = SNlast + i;
					}
				}
			}
			
		}

	//CRC valid and nak received
	}else if(crc_check == 0 && packetType == 3){
		//SN is in window
		if(SN >= SNlast && SN <= (SNlast + WINDOW - 1)){
			printf("Received NAK >> Type: %02d, SN: %02d, D1: %02d, D2: %02d, CRC: %02X%02X\n\r", srv_reply[0], srv_reply[1], srv_reply[2], srv_reply[3],srv_reply[4], srv_reply[5]);
			SNlast = SN;

			//resend all packets in window
			for(i = 0; i < WINDOW && (SNlast + i) < 13; i++){
				data[0] = alphabet[(2 * (SNlast+ i))];
				data[1] = alphabet[(2 * (SNlast+ i)) + 1];
				data[2] = '\0';
				generatePacket(data,SNlast+ i,ber);
				if( send(sockfd , packet, 6, 0) < 0)
					perror("Send failed");
				window[i] = SNlast + i;
			}
		}

	//Invalid ack/nak packet so discard it
	}else{
		printf("Discarded ack/nak\n\r");
	}
	
    }
}

//Generate data packet, assumes cData is 2 bytes (2 characters)
void generatePacket(unsigned char cData[], unsigned char packetNumber, double ber){
	
	packet[0] = 1;
	packet[1] = packetNumber;
	packet[2] = cData[0];
	packet[3] = cData[1];
	packet[4] = '\0';
	
	//Generate crc and split into two bytes
	crc = calculate_CCITT16(packet, 2, GENERATE_CRC);
	crc_upper = (uint8_t) (crc >> 8);
	crc_lower = (uint8_t) (crc);

	packet[4] = crc_upper;
	packet[5] = crc_lower;
	
	//Add error to packet passed in via command line argument
	IntroduceError(packet,ber);

	//Increment number of packets sent to receiver
	numPackets = numPackets + 1;

	//Print out packet
	printf("Sent         >> Type: %02d, SN: %02d, D1: %02c, D2: %02c, CRC: %02X%02X, Packet: %d\n\r", packet[0], packet[1], packet[2], packet[3],packet[4], packet[5], numPackets);

}

long M = 2147483647;
//Add error to packet given BER
void IntroduceError(char *data, double p)
{
	char c, *pointer = data;
	int i;
	while (*pointer != '\0') {
		c = 0x01;
		for ( i = 0; i < 8; i++) {
			if ((double)random()/M <= p)
				*pointer ^= c;
			c <<= 1;
		}
		pointer++;
	}
}



