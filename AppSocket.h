//
// Created by lanshuhan on 19-6-2.
//

#ifndef TLSUDPPROTOCOL_APPSOCKET_H
#define TLSUDPPROTOCOL_APPSOCKET_H

#include"SecureSocket.h"

#define DATA_SIZE 1000
#define READ_BUF_SIZE 1010

#define STRING_MAX 30000

struct packet
{
    unsigned short header = 0;
    unsigned int payloadLength = 0;
    unsigned int packetID = 0;
    char* body = nullptr;
};

packet parsePacket(char *pac);

char * deparsePacket(packet fpac);

packet getShortPacket(unsigned short type);

packet getMediumPacket(unsigned short type, unsigned int length, char* content);

packet getLongPacket(unsigned short type, unsigned int length, unsigned int pID, char* content);

char* getCharPacket(unsigned short type);

char* getCharPacket(unsigned short type, unsigned int length, char* content);

char* getCharPacket(unsigned short type, unsigned int length, unsigned int pID, char* content);

void recvPacket(char* target, unsigned short& type, unsigned int& length, unsigned int& pID, char* content);

bool checkPassword(char* pw3, int len3, char* pw, int len);

char* sha1(char* src);

class AppSocket : public SecureSocket
{
private:

public:
    AppSocket(const char *configPath = "./config.json");

    AppSocket(unsigned short localPort, const char *configPath = "./config.json");

    AppSocket(const string &localAddress, unsigned short localPort,
              const char *configPath = "./config.json");

    ~AppSocket();

    //passCat = 3 * password, connect with ','
    int connectToServer(const char* passCat, int passLen, const char* add);

    int connectToClient(const char* pass, int passLen, const char* add);
};


#endif //TLSUDPPROTOCOL_APPSOCKET_H
