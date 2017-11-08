//
//  Code by Ya ZUO, ID: W1344246; May 29, 2017
//  time out
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


const uint16_t StartOfPacketIdentifier = 0xFFFF;
const uint16_t EndOfPacketIdentifier = 0xFFFF;
const uint8_t ClientId = 0xCC;
const uint8_t Length = 0xFF;
const uint8_t WrongLength = 0xFA;
const uint16_t DATA = 0XFFF1;
const uint16_t ACK = 0XFFF2;
const uint16_t REJECT = 0XFFF3;
const uint16_t REJECTOutOfSequence = 0XFFF4;
const uint16_t REJECTLengthMismatch = 0XFFF5;
const uint16_t REJECTEndOfPacketMissing = 0XFFF6;
const uint16_t REJECTDuplicatePacket = 0XFFF7;


void ResponseCheck(char* packet);
char* sendPacket(int sockClient, char* packet, int packetSize, struct sockaddr_in server);
char* pack(int segNo, int* cnt);
/****** Function main**************************************************/
int main(int argc, char **argv){
    int sockClient;
    struct sockaddr_in server, client;
    int s_len = sizeof(client);;
    struct hostent *hp;
    struct timeval tv;
    
    char *packet;
    int packetSize = 0;
    char* recBuf;
    
    
    /*
     Create a socket of client.
     AF_INET says that it will be an Internet socket.
     SOCK_DGRAM says that it will use datagram delivery instead of virtual circuits.
     IPPROTO_UDP says that it will use the UDP protocol
     */
    sockClient = socket(AF_INET, SOCK_DGRAM, 0);
    

    client.sin_family = AF_INET;
    client.sin_port = htons(22345);
    hp = gethostbyname("localhost");
    bcopy( hp->h_addr, &(client.sin_addr.s_addr), hp->h_length);
    
    /* Next line tells the system that the socket s should be bound to the address in si_me.*/
    bind( sockClient, (struct sockaddr *) &client, sizeof(client));
    

    server.sin_family = AF_INET;
    server.sin_port = htons(12345);
    bcopy( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    
    /* ---- Setup non blocking io, and specify the duration ---- */
    tv.tv_usec = 0;
    tv.tv_sec = 2;
    setsockopt(sockClient, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval));
    
    
    /* for loop here is used to send package to server.
     If client doesn't receive ACK from server
     resend the mesage 3 times to server
     */
    for(int i = 0; i < 5; i++){
        printf("Send packet %d to Server.\n", i+1);
        
        packet = pack(i+1, &packetSize);
        
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
        
        recBuf = sendPacket(sockClient, packet, packetSize, server);
        if(recBuf != NULL){
            ResponseCheck(recBuf);
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
void ResponseCheck(char* packet){
    
    /* ---- parse the packet according to the packet strcuture ---- */
    // start
    uint16_t startOfPacket;
    int shift = 0;
    memcpy(&startOfPacket, packet + shift, sizeof(startOfPacket));
    //printf("This is start : %x\n", startOfPacket);
    
    // id
    uint8_t ClientId;
    shift += sizeof(startOfPacket);
    memcpy(&ClientId, packet + shift, sizeof(ClientId));
    //printf("This is ClientId : %x\n", ClientId);
    
    // ACK or reject
    uint16_t rejOrACK;
    shift += sizeof(ClientId);
    memcpy(&rejOrACK, packet + shift, sizeof(rejOrACK));
    //printf("This is rejOrACK : %x\n", rejOrACK);
    
    // seg no
    char packSegNo;
    memcpy(&packSegNo, packet + shift, sizeof(packSegNo));
    //printf("This is packSegNo : %x\n", packSegNo);
    shift += sizeof(packSegNo);
    
    // end
    uint16_t endOfPacket;
    memcpy(&endOfPacket, packet + shift, sizeof(endOfPacket));
    //printf("This is endOfPacket : %x\n", endOfPacket);
    
}

/*****Function_sendPackage************************************************************************/
/* -Function: sendPackage:
 client sends packets to server
 If there is no response,
 resend packets for three times*/
char* sendPacket(int sockClient, char* packet, int packetSize, struct sockaddr_in server){
    int size = 10;
    char* recBuf = malloc(size * sizeof(char));
    
    for(int i = 0; i < 4; i++){
        sendto(sockClient, packet, packetSize, 0, (struct sockaddr *) &server, sizeof(server) );
        int rc = recv(sockClient, recBuf, size, 0);
        
        if(rc>0){
            return recBuf;
        }else if(i!=0 && rc <= 0){
            printf("Resend packet to server %d\n", i);
        }
    }
    return NULL;
}

/* ---- pack is used for building the packet in the specific format ---- */
char* pack(int segNo, int* cnt){
    
    *cnt = sizeof(StartOfPacketIdentifier)+
    sizeof(ClientId)+
    sizeof(DATA)+
    1+
    sizeof(Length)+
    (int)Length+
    sizeof(EndOfPacketIdentifier);
    
    char *packet = malloc(*cnt * sizeof(char));
    
    // start
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // id
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &ClientId, sizeof(ClientId));
    
    // data type
    shift += sizeof(ClientId);
    memcpy(packet + shift, &DATA, sizeof(DATA));
    
    // seg no
    shift += sizeof(DATA);
    *(packet + shift) = segNo;
    
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
