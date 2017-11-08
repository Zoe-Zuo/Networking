//
//  Code by Ya ZUO, ID: W1344246; May 29, 2017
//
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> 
#include <stdint.h>
#include <sys/time.h>
#include <stdint.h>

#define BUFLEN 14// bufer size 2+1+2+1+1+1+4+2=14
#define PORT 2222 // port number

/* Use constant to represent */
const uint16_t StartOfPacketIdentifier = 0xFFFF;
const uint16_t EndOfPacketIdentifier = 0xFFFF;
const uint8_t ClientId = 0xCC;
const uint8_t Length = 0xFF;
const uint16_t DATA = 0XFFF1;
const uint16_t ACK = 0XFFF2;

const uint16_t Acc_Per = 0xFFF8;
const uint8_t _2G = 0x02;
const uint8_t _3G = 0x03;
const uint8_t _4G = 0x04;
const uint16_t Not_paid = 0xFFF9;
const uint16_t Not_exist = 0xFFFA;
const uint16_t Access_OK = 0xFFFB;

//3 users and their information
struct Database{
    uint32_t number;
    uint8_t Technology;
    int paid;
} ;

extern const struct Database users[3];

char* sendPacket(int sockClient, char* packet, int packetSize, struct sockaddr_in server, int* size);
char* pack(int segNo, uint32_t SourceSubscriberNo, uint8_t Technology, int* cnt);
void TeleResponseCheck(char* packet, int size);

int main(int argc, char **argv){
    int sockClient;
    struct sockaddr_in server, client;
    int s_len = sizeof(client);;
    struct hostent *hp;
    struct timeval tv;
    
    char *packet;
    int packetSize = 0;
    char* recBuf;
    int recBufSize = 0;
    
    //creat socket
    sockClient = socket(AF_INET, SOCK_DGRAM, 0);
    
    
    client.sin_family = AF_INET;
    client.sin_port = htons(PORT);
    hp = gethostbyname("localhost");
    bcopy( hp->h_addr, &(client.sin_addr.s_addr), hp->h_length);

    //bind
    bind( sockClient, (struct sockaddr *) &client, sizeof(client));
    
    
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    bcopy( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    
    //timer = 3 sec
    tv.tv_usec = 0;
    tv.tv_sec = 2;
    setsockopt(sockClient, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval));
    
    //Check users from database
    for(int i = 0; i < 3; i++){
        printf("\n Whether user %d is in the database.\n", i+1);
        packet = pack(i+1, users[i].number, users[i].Technology, &packetSize);
        recBuf = sendPacket(sockClient, packet, packetSize, server, &recBufSize);
        if(recBuf != NULL){
            TeleResponseCheck(recBuf, recBufSize);
        }else{
            printf("Server does not respond.\n");
            break;
        }
    }
    
    printf("End of the program\n");	
    
    return 0;
    
}

//Database
const struct Database users[3] = {{0xFFFFFFF1, _4G, 1}, {0xFFFFFF12, _3G, 0}, {0x1111111F, _2G, 1}}; // 1 paid 0 unpaid


//Send packet, if no response, resend three times.
char* sendPacket(int sockClient, char* packet, int packetSize, struct sockaddr_in server, int *size){
    int recBufSize = 300;
    char* recBuf = malloc(recBufSize * sizeof(char));
    
    for(int i = 0; i < 3; i++){
        sendto(sockClient, packet, packetSize, 0, (struct sockaddr *) &server, sizeof(server) );
        int rc = recv(sockClient, recBuf, recBufSize, 0);
        *size = rc;
        if(rc>0){
            return recBuf;
        }else if(i!=0 && rc <= 0){
            printf("Resend packet to server %d\n", i);
        }
    }
    return NULL;
}

//Packet format
char* pack(int segNo, uint32_t SourceSubscriberNo, uint8_t Technology, int* cnt){
    
    *cnt = 14;
    
    char *packet = malloc(*cnt * sizeof(char));
    
    // start
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // id
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &ClientId, sizeof(ClientId));
    
    // Acc_Per
    shift += sizeof(ClientId);
    memcpy(packet + shift, &Acc_Per, sizeof(Acc_Per));
    
    // seg no
    shift += sizeof(Acc_Per);
    *(packet + shift) = segNo;
    
    // length
    shift++;
    memcpy(packet + shift, &Length, sizeof(Length));
    
    // Technology
    shift += sizeof(Length);
    memcpy(packet + shift, &Technology, sizeof(Technology));
    
    // SourceSubscriberNo
    shift += sizeof(Technology);
    memcpy(packet + shift, &SourceSubscriberNo, sizeof(SourceSubscriberNo));
    
    // end
    shift += ((int)Length - sizeof(Technology));
    memcpy(packet + shift, &EndOfPacketIdentifier, sizeof(EndOfPacketIdentifier));
    
    return packet;
}

//Responses to the client
void TeleResponseCheck(char* packet, int size){
    
    // Acc_Per
    uint16_t Access;
    memcpy(&Access, packet + 3, sizeof(Access));
    switch(Access){
        case Not_paid:
            printf("Sorry, you have not paid.\n");
            break;
        case Not_exist:
            printf("Sorry, your number does not exist in our database.\n");
            break;
        case Access_OK:
            printf("Welcome, you are permitted to access!\n");
            break;
    }
    
    
    // Technology(1G->4G)
    uint8_t Technology;
    memcpy(&Technology, packet + 7, sizeof(Technology));
    printf("Technology Using: %x G\n", Technology);
    
    // SubscriberNo
    uint32_t SubscriberNo;
    memcpy(&SubscriberNo, packet + 8, sizeof(uint32_t));
    printf("The number of Subscriber: %ld\n", (long)SubscriberNo);
    printf("************************************************ \n");
    
}

