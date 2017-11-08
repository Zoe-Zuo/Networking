//
//  Code by Ya ZUO, ID: W1344246; May 29, 2017
//

#include <stdlib.h>// This header is for exit(1);
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <stdint.h>
#include <sys/time.h>
#include <stdint.h>

#define BUFLEN 264// bufer size 2+1+2+1+1+255+2=264
#define NPACK 5//number of package to receive
#define PORT 2222 // port number

/* Use constant to represent:
 1. Design a protocol with following primitives
 */
const uint16_t StartOfPacketIdentifier = 0xFFFF;
const uint16_t EndOfPacketIdentifier = 0xFFFF;
const uint8_t ClientId = 0xCC;
const uint8_t Length = 0xFF;
/* Use constant to represent:
 2. Packet Types
 */
const uint8_t WrongLength = 0xFA;
const uint16_t DATA = 0XFFF1;
const uint16_t ACK = 0XFFF2;
/* Use constant to represent:
 3. Reject Types
 */
const uint16_t REJECT = 0XFFF3;
const uint16_t REJECTOutOfSequence = 0XFFF4;
const uint16_t REJECTLengthMismatch = 0XFFF5;
const uint16_t REJECTEndOfPacketMissing = 0XFFF6;
const uint16_t REJECTDuplicatePacket = 0XFFF7;


enum result
{
    Sent = 0,
    OutOfSequence,
    LengthMissmatch,
    EndOfPacketMissing,
    DuplicatePacket
};

/* Use three functions to check the packages and error handling */

char* ErrorPackage(uint8_t ID, char segmentNo, uint16_t rejectSubCode, int* count);
char* ACKPackage(uint8_t ID, char segmentNo, int* cnt);
int DataPacketFormatMatch(char* packet, int size, uint8_t *ID, char *segmentNo);

void diep(char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char **argv){
    
    struct sockaddr_in si_me, si_other;//sockaddr_in is a structure containing an Internet socket address
    /*
     si_me stands for server, si_other stands for client
     an address family (always AF_INET for our purposes)
     a port number
     an IP address
     si_me defines the socket where the server will listen.
     si_other defines the socket at the other end of the link (that is, the client).
     */
    int s;
    int i;
    int receive;
    int s_len = sizeof(si_other);// Then length of the client
    //int rv;
    char buf[BUFLEN];//char packet[264];
    char* responseTo;
    int responseSize;
    uint8_t ID;
    char segmentNo;
    int segmentC = 0;
    int lastSegmentNo = -1;
    memset(buf, 0 ,sizeof(buf));
    
    /*
     Create a socket.
     AF_INET says that it will be an Internet socket.
     SOCK_DGRAM says that it will use datagram delivery instead of virtual circuits.
     IPPROTO_UDP says that it will use the UDP protocol
     */
    
    s = socket(AF_INET, SOCK_DGRAM, 0);
    
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        diep("socket");
    }
    
    
    si_me.sin_family = AF_INET;//We will use Internet addresses
    si_me.sin_port = htons(PORT);/*Here, the port number is defined.htons() ensures that the byte order is correct*/
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);/*This line is used to tell what IP address we want to bind to.*/
    
    /* Next line tells the system that the socket s should be bound to the address in si_me.*/
    bind( s, (struct sockaddr *) &si_me, sizeof(si_me));
    
    // The server is waiting forz the package from client
    while(1){
        
        receive = recvfrom(s, &buf, sizeof(buf), 0, (struct sockaddr *) &si_other, (socklen_t *) &s_len);
        
        /* ---- send the message back to client ---- */
        printf("A packet came from client. \n");
        int result = DataPacketFormatMatch(buf, receive, &ID, &segmentNo);
        
        //printf("Result code %d\n", result);
        
        /* ---- First, switch will check the packet length and the end identifier. ---- */
        /* ---- Second, if statement inside the OK case will check the sequence No. and duplicate. ---- */
        /* ---- Finally, if all the check point passed, will send ACK back to user. ---- */
        if(result == Sent){
         segmentC++;
         if((int)segmentNo == lastSegmentNo){
         printf("Duplicate!\n");
         responseTo = ErrorPackage(ID, segmentNo, REJECTDuplicatePacket, &responseSize);
         sendto(s, responseTo, responseSize, 0, (struct sockaddr *) &si_other, sizeof(si_other) );
         }
         
         if(segmentC == (int)segmentNo){
         printf("Well received.\n");
         responseTo = ACKPackage(ID, segmentNo, &responseSize);
         sendto(s, responseTo, responseSize, 0, (struct sockaddr *) &si_other, sizeof(si_other) );
         if(segmentC>5){
         segmentC = 1;
         }
         }else{
         printf("Wrong Sequence!\n");
         responseTo = ErrorPackage(ID, segmentNo, REJECTOutOfSequence, &responseSize);
         sendto(s, responseTo, responseSize, 0, (struct sockaddr *) &si_other, sizeof(si_other) );
         }
         lastSegmentNo = (int)segmentNo;
         
         }
         if(result == LengthMissmatch){
         printf("Wrong Length!\n");
         responseTo = ErrorPackage(ID, segmentNo, REJECTLengthMismatch, &responseSize);
         sendto(s, responseTo, responseSize, 0, (struct sockaddr *) &si_other, sizeof(si_other) );
         
         }
         if(result == EndOfPacketMissing){
         printf("End Of Packet is Missing!\n");
         responseTo = ErrorPackage(ID, segmentNo, REJECTEndOfPacketMissing, &responseSize);
         sendto(s, responseTo, responseSize, 0, (struct sockaddr *) &si_other, sizeof(si_other) );
         
         
         }
         printf("****************************** \n");
         }
         
    
    /* ---- close socket ---- */
    return 0;
    
}

/******Function_ErrorPackage**********************************************************************/

char* ErrorPackage(uint8_t ID, char segmentNo, uint16_t rejectSubCode, int* count){
    
    *count = sizeof(StartOfPacketIdentifier)+sizeof(ID)+sizeof(REJECT)+sizeof(rejectSubCode)+sizeof(segmentNo)+sizeof(EndOfPacketIdentifier);
    
    char *packet = malloc(*count * sizeof(char));
    
    // StartOfPacketIdentifier
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // ID
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &ID, sizeof(ID));
    
    // REJECT
    shift += sizeof(ID);
    memcpy(packet + shift, &REJECT, sizeof(REJECT));
    
    // REJECT subcode
    shift += sizeof(REJECT);
    memcpy(packet + shift, &rejectSubCode, sizeof(rejectSubCode));
    
    // Received segment No.
    shift += sizeof(rejectSubCode);
    memcpy(packet + shift, &segmentNo, sizeof(segmentNo));
    //End Of Packet Identifier End
    shift += sizeof(segmentNo);
    memcpy(packet + shift, &EndOfPacketIdentifier, sizeof(EndOfPacketIdentifier));
    
    return packet;
}
/******Function_ACKPackage*************************************************/
char* ACKPackage(uint8_t ID, char segmentNo, int* countACK){
    
    *countACK = sizeof(StartOfPacketIdentifier)+sizeof(ID)+sizeof(ACK)+sizeof(segmentNo)+sizeof(EndOfPacketIdentifier);
    
    char *packet = malloc(*countACK * sizeof(char));
    
    // StartOfPacketIdentifier
    int shift = 0;
    memcpy(packet + shift, &StartOfPacketIdentifier, sizeof(StartOfPacketIdentifier));
    
    // ID
    shift += sizeof(StartOfPacketIdentifier);
    memcpy(packet + shift, &ID, sizeof(ID));
    
    // ACK
    shift += sizeof(ID);
    memcpy(packet + shift, &ACK, sizeof(ACK));
    
    // Received segment No.
    shift += sizeof(ACK);
    memcpy(packet + shift, &segmentNo, sizeof(segmentNo));
    
    // End
    shift += sizeof(segmentNo);
    memcpy(packet + shift, &EndOfPacketIdentifier, sizeof(EndOfPacketIdentifier));
    
    return packet;
}
/*******Function_DataPacketFormatMatch*******************************************************/
int DataPacketFormatMatch(char* packet, int size, uint8_t *ID, char *segNo){
    
    // start of packet
    uint16_t startOfPacket;
    int shift = 0;
    memcpy(&startOfPacket, packet + shift, sizeof(startOfPacket));
    printf("Start Of Packet : %X\n", startOfPacket);
    
    // ClientID
    uint8_t ClientId;
    shift += sizeof(startOfPacket);
    memcpy(&ClientId, packet + shift, sizeof(ClientId));
    *ID = ClientId;
    printf("Client Id : %X\n", ClientId);
    
    // Data Type
    uint16_t dataType;
    shift += sizeof(ClientId);
    memcpy(&dataType, packet + shift, sizeof(dataType));
    printf("Data Type : %X\n", dataType);
    
    // segment No.
    char packSegmentNo;
    shift += sizeof(dataType);
    memcpy(&packSegmentNo, packet + shift, sizeof(packSegmentNo));
    *segNo = packSegmentNo;
    printf("Packet Segment No : %X\n", packSegmentNo);
    
    // length
    uint8_t Length;
    shift += sizeof(packSegmentNo);
    memcpy(&Length, packet + shift, sizeof(Length));
    printf("Length : %X\n", Length);
    
    // payload
    shift += sizeof(Length);
    int payloadStart = shift;
    
    
    // end of packet
    uint16_t endOfPacket;
    int payloadEnd = size - sizeof(endOfPacket);
    //printf("End of payload: %d\n", payloadEnd);
    memcpy(&endOfPacket, packet + payloadEnd, sizeof(endOfPacket));
    printf("End Of Packet : %X\n", endOfPacket);
    printf("-------- \n");

    
    // the size of payload
    if(payloadEnd - payloadStart != (int) Length){
        return LengthMissmatch;
    }
    
    // end of packet Identifier
    if(endOfPacket != EndOfPacketIdentifier){
        return EndOfPacketMissing;
    }
    
    // already sent packets
    return Sent;    
}
