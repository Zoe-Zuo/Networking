//
//  Code by Ya ZUO, ID: W1344246; May 29, 2017
//  Wrong Sequence
//


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>

#define BUFLEN 264// bufer size 2+1+2+1+1+255+2=264
#define NPACK 5//number of package to receive
#define PORT 2222 // port number


/* Use constant to represent:
 1. Design a protocol with following primitives
 */
const uint16_t StartOfPacketIdentifier = 0xFFFF;
const uint16_t EndOfPacketIdentifier = 0xFFFF;
const uint8_t  ClientId = 0xCC;
const uint8_t  Length = 0xFF;
/* Use constant to represent:
 2. Packet Types
 */
const uint8_t  WrongLength = 0xFA;
const uint16_t DATA = 0XFFF1;
const uint16_t ACK = 0XFFF2;
const uint16_t REJECT = 0XFFF3;
/* Use constant to represent:
 3. Reject Types
 */

const uint16_t REJECTOutOfSequence = 0XFFF4;
const uint16_t REJECTLengthMismatch = 0XFFF5;
const uint16_t REJECTEndOfPachetMissing = 0XFFF6;
const uint16_t REJECTDuplicatePacket = 0XFFF7;
/* functions used to do
 (1)response check
 (2)send package to server
 (3)the definition of package */

void  CheckToResponse(char* packet);
char* sendPackage(int sockClient, char* packet, int packetSize, struct sockaddr_in sin_me);
char* specificFormatOfPack(int segmentNo, int* count);

/****************************Function:main****************************************/
int main(int argc, char **argv){
    int s;//This is the socket for client
    struct sockaddr_in sin_me, sin_other;// sin_me represents server and sin_other represents client
    int s_len = sizeof(sin_other);;
    
    char *packet;
    int  packetSize = 0;
    char* recBuf;
    
    struct hostent *hp;
    struct timeval tv;
    
    
    
    
    s = socket(AF_INET, SOCK_DGRAM, 0);
    
    /*
     Create a socket of client.
     AF_INET says that it will be an Internet socket.
     SOCK_DGRAM says that it will use datagram delivery instead of virtual circuits.
     IPPROTO_UDP says that it will use the UDP protocol
     */
    
    sin_other.sin_family = AF_INET;
    sin_other.sin_port = htons(PORT);
    hp = gethostbyname("localhost");
    bcopy( hp->h_addr, &(sin_other.sin_addr.s_addr), hp->h_length);
    
    /* Next line tells the system that the socket s should be bound to the address in si_me.*/
    bind( s, (struct sockaddr *) &sin_other, sizeof(sin_other));
    
    
    sin_me.sin_family = AF_INET; // server
    sin_me.sin_port = htons(PORT);//server
    bcopy( hp->h_addr, &(sin_me.sin_addr.s_addr), hp->h_length);
    
    //timer = 3 seconds
    tv.tv_usec = 0;
    tv.tv_sec = 2;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval));
    
    
    /* for loop here is used to send package to server.
     If client doesn't receive ACK from server
     resend the mesage 3 times to server
     */
    for(int i = 0; i < NPACK; i++){
        printf("Send packet %d to Server.\n", i+1);
        
        packet = specificFormatOfPack(i+1, &packetSize);
        recBuf = sendPackage(s, packet, packetSize,sin_me);

        // ---- Error Test: Not in sequence
        /*if(i == 4){
         printf("Change the pack sequence.\n");
         packet = pack(i+2, &packetSize);
         }*/
        
        // ---- Error Test: duplicate
        /*if(i == 2){
         printf("Change the pack sequence.\n");
         packet = pack(i, &packetSize);
         }*/

        //Errror with wrong sequence
         if(i == 0){
         printf("Change the pack sequence.\n");
             i=i+2;
         
         }
         
         
        
        if(recBuf != NULL){
            CheckToResponse(recBuf);
        }else{
            printf("Server does not respond.\n");
            break;
        }
    }
    
    
    printf("End of the program\n");
    
    return 0;
    
}
/********Function_CheckToResponse******************************************************/
/*Function CheckToResponse
 is used to check the response from server
 */
void CheckToResponse(char* packet){
    
    
    /*Start of package ID*/
    uint16_t startOfPacket;//Length = 2 bytes
    int shift = 0;
    memcpy(&startOfPacket, packet + shift, sizeof(startOfPacket));
    printf("Start of package: %X\n", startOfPacket);
    // printf("******************************");
    /*Client ID*/
    uint8_t ClientID;//Length = 1byte
    shift += sizeof(startOfPacket);// Move from the part 1（StartOfPacket）
    memcpy(&ClientID, packet + shift, sizeof(ClientID));
    printf("Client ID : %X\n", ClientID);
    
    /*DATA stands for ACK or reject information*/
    uint16_t DATA;
    shift += sizeof(ClientID);
    memcpy(&DATA, packet + shift, sizeof(DATA));
    printf("Received from server : %X\n", DATA);
    
    // seg no
    char packSegmentNo;
    memcpy(&packSegmentNo, packet + shift, sizeof(packSegmentNo));
    //printf("PackSegmentNo : %X\n", packSegmentNo);
    shift += sizeof(packSegmentNo);
    
    // end
    uint16_t endOfPacket;
    memcpy(&endOfPacket, packet + shift, sizeof(endOfPacket));
    //printf("End of Packet : %X\n", endOfPacket);
    
    /*reject error handling with subcode*/
    if(DATA == REJECT){
        uint16_t subcode;
        shift += sizeof(DATA);
        memcpy(&subcode, packet + shift, sizeof(subcode));
        switch(subcode){
            case REJECTOutOfSequence:
                printf("REJECT Out Of Sequence\n");
                break;
            case REJECTLengthMismatch:
                printf("REJECT Length Mismatch\n");
                break;
            case REJECTEndOfPachetMissing:
                printf("REJECT End Of Pachet Missing\n");
                break;
            case REJECTDuplicatePacket:
                printf("REJECT Duplicate Packet\n");
                break;
        }
        shift += sizeof(subcode);
    }else{
        printf("Success!!!Receive the ACK from server.\n");
        shift += sizeof(DATA);
    }
    printf("****************************** \n");
    
}
/*****Function_sendPackage************************************************************************/
/* -Function: sendPackage:
 client sends packets to server
 If there is no response,
 resend packets for three times*/
char* sendPackage(int sockClient, char* packet, int packetSize, struct sockaddr_in sin_me){
    int size = 10;
    char* recBuf = malloc(size * sizeof(char));
    //test code_case 1: REJECTOutOfSequence
    
    
    for(int i = 0; i < 4; i++){
        sendto(sockClient, packet, packetSize, 0, (struct sockaddr *) &sin_me, sizeof(sin_me) );
        int rc = recv(sockClient, recBuf, size, 0);
        
        if(rc>0){
            return recBuf;
        }else if(i!=0 && rc <= 0){
            printf("Resend packet to server %d\n", i);
        }
    }
    return NULL;
}
/*******Function_specificFormatOfPack*****************************************************************/
/* It's used to build the format just like what is written on the assigment paper  */
char* specificFormatOfPack(int segmentNo, int* count){
    
    *count = sizeof(StartOfPacketIdentifier)+sizeof(ClientId)+sizeof(DATA)+1+sizeof(Length)+(int)Length+sizeof(EndOfPacketIdentifier);
    
    char *packet = malloc(*count * sizeof(char));
    
    // StartOfPacketIdentifier
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // ClientId
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &ClientId, sizeof(ClientId));
    
    // data type
    shift += sizeof(ClientId);
    memcpy(packet + shift, &DATA, sizeof(DATA));
    
    // segment No
    shift += sizeof(DATA);
    *(packet + shift) = segmentNo;
    
    // length
    shift++;
    // ---- Error Test: Length mismatch
    //memcpy(packet + shift, &WrongLength, sizeof(WrongLength));
    memcpy(packet + shift, &Length, sizeof(Length));
    
    // payload
    shift += sizeof(Length);
    // do nothing
    
    // end
    // ---- Error Test: Lost of end identifier
    shift += (int)Length;
    memcpy(packet + shift, &EndOfPacketIdentifier, sizeof(EndOfPacketIdentifier));
    
    return packet;
}
