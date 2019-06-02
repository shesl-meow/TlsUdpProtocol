//
// Created by lanshuhan on 19-6-2.
//

#ifndef TLSUDPPROTOCOL_APPSOCKET_H
#define TLSUDPPROTOCOL_APPSOCKET_H

#include "SecureSocket.h"

#define DATA_SIZE 1000
#define READ_BUF_SIZE 1010

#define STRING_MAX 30000


/**
  *   Implement a app socket from secure socket
  */
class AppSocket : public SecureSocket
{
private:
    /**
     * App socket format, with three header.
     * More specific description about the bits message was list on README.md
     */
    struct formatPacket
    {
        unsigned short header = 0;
        unsigned int payloadLength = 0;
        unsigned int packetID = 0;
        char* body = nullptr;
    };

    /**
     * Transfer a char array to a formatted packet struct, remember release memory.
     * @param packet a char array from the peer side
     * @return a formatted packet struct
     */
    static formatPacket parsePacket(const char *pac);

    /**
     * Trasfer a formatted packet struct to a char array, remember release memory.
     * @param fpacket a formatted packet
     * @return a char array to send to peer side
     */
    static char* deparsePacket(formatPacket pac);

    formatPacket getShortPacket(unsigned short type);

    formatPacket getMediumPacket(unsigned short type, unsigned int length, char* content);

    formatPacket getLongPacket(unsigned short type, unsigned int length, unsigned int pID, char* content);

    char* getCharPacket(unsigned short type);

    char* getCharPacket(unsigned short type, unsigned int length, char* content);

    char* getCharPacket(unsigned short type, unsigned int length, unsigned int pID, char* content);

public:
    /**
     *   Construct a app socket
     */
    explicit AppSocket(const char *configPath);

    /**
     *   Construct a app socket with the given local port
     *   @param localPort local port
     */
    AppSocket(unsigned short localPort, const char *configPath);

    /**
     *   Construct a app socket with the given local port and address
     *   @param localAddress local address
     *   @param localPort local port
     */
    AppSocket(const string &localAddress, unsigned short localPort, const char *configPath);

    ~AppSocket();

    //passCat = 3 * password, connect with ','
    int connectToServer(const char* passCat, int passLen, const char* add);

    int connectToClient(const char* pass, int passLen, const char* add);

    void recvPacket(char* target, unsigned short& type, unsigned int& length, unsigned int& pID, char* content);

    bool checkPassword(const char* pw3, int len3, const char* pw, int len);

    char* sha1(char* src);
};


#endif //TLSUDPPROTOCOL_APPSOCKET_H
