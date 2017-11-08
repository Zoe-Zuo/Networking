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
    int paid;// 0 or 1
} ;

extern const struct Database users[2];//3 users

enum result
{
    Send = 0,
    NotPaid,
    NotExist,
    Permitted
};

int DataPacketFormatMatch(char* packet, int size, uint8_t *id, char *segNo, uint8_t* Technology, uint32_t* SubscriberNo);
char* ACKSubscriber(uint8_t id, char segNo, uint16_t result, uint8_t Technology, uint32_t SubscriberNo, int* cnt);

int main(int argc, char **argv){
    int s;
    struct sockaddr_in server, client;
    int s_len = sizeof(client);;
    int rc;
    
    char packet[BUFLEN];
    char* response;
    int responseSize;
    memset(packet, 0 ,sizeof(packet));
    uint8_t id;
    char segNo;
    int segCnt = 0;
    int lastSegNo = -1;
    
    uint32_t SourceSubscriberNo;
    uint8_t Technology;
    
    //Create socket
    s = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind
    bind(s, (struct sockaddr *) &server, sizeof(server));
    
    
    while(1){
        int rv = recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *) &client, (socklen_t *) &s_len);
        
        //receive from client
        printf("************************************************ \n");
        printf("Receive a packet from the client!\n");
        int resultCode = DataPacketFormatMatch(packet, rv, &id, &segNo, &Technology, &SourceSubscriberNo);
        
        //check the access permission
        switch (resultCode){
            case Send:
                segCnt++;
            case Permitted:
                printf("Permitted!\n");
                response = ACKSubscriber(id, segNo, Access_OK, Technology, SourceSubscriberNo, &responseSize);
                sendto(s, response, responseSize, 0, (struct sockaddr *) &client, sizeof(client) );
                break;
            case NotPaid:
                printf("Not paid!\n");
                response = ACKSubscriber(id, segNo, Not_paid, Technology, SourceSubscriberNo, &responseSize);
                sendto(s, response, responseSize, 0, (struct sockaddr *) &client, sizeof(client) );
                break;
            case NotExist:
                printf("Number not exist!\n");
                response = ACKSubscriber(id, segNo, Not_exist, Technology, SourceSubscriberNo, &responseSize);
                sendto(s, response, responseSize, 0, (struct sockaddr *) &client, sizeof(client) );
                break;
        }
    }
    
    /* ---- close socket ---- */
    return 0;
    
}



//Subscriber Database
const struct Database users[2] = {{0xFFFFFFF1, _4G, 1}, {0xFFFFFF12, _3G, 0}}; // 1 paid 0 unpaid

int DataPacketFormatMatch(char* packet, int size, uint8_t *id, char *segNo, uint8_t* Technology, uint32_t* SubscriberNo){    
    
    // Technology
    memcpy(Technology, packet + 7, sizeof(uint8_t));
    printf("Technology Using: %x G\n", *Technology);
    
    // SourceSubscriberNo
    memcpy(SubscriberNo, packet + 8, sizeof(uint32_t));
    printf("Subscriber Number: %ld\n", (long)*SubscriberNo);
    
    // check user request
    for(int i = 0; i < sizeof(users); i++){
        if(*SubscriberNo == users[i].number){
            if(*Technology == users[i].Technology){
                if(users[i].paid){
                    return Permitted;
                }else{
                    return NotPaid;
                }
            }else{
                return NotExist;
            }
        }
    }
    return NotExist;
}

char* ACKSubscriber(uint8_t id, char segNo, uint16_t result, uint8_t Technology, uint32_t SubscriberNo, int* cnt){
    
    *cnt = 14;
    
    char *packet = malloc(*cnt * sizeof(char));
    
    // Start Of Packet Identifiert
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // ID
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &id, sizeof(id));
    
    // result
    shift += sizeof(id);
    memcpy(packet + shift, &result, sizeof(result));
    
    // Received segment number.
    shift += sizeof(result);
    memcpy(packet + shift, &segNo, sizeof(segNo));
    
    // length
    shift += sizeof(segNo);
    memcpy(packet + shift, &Length, sizeof(Length));
    
    // Technology(1G->4G)
    shift += sizeof(Length);
    memcpy(packet + shift, &Technology, sizeof(Technology));
    
    // SubscriberNo
    shift += sizeof(Technology);
    memcpy(packet + shift, &SubscriberNo, sizeof(SubscriberNo));
    
    // end of packet Identifier
    shift += ((int)Length - sizeof(Technology));
    memcpy(packet + shift, &EndOfPacketIdentifier, sizeof(EndOfPacketIdentifier));
    
    return packet;
}

