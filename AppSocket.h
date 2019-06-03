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

    /**
     * get sha1 hash value of a string with `openssl` library.
     * @param srcBuffer source buffer
     * @param srcSize source buffer size
     * @param destBuffer destination buffer size with 20 bytes fix array.
     */
    void getHashValue(const char *srcBuffer, size_t srcSize, unsigned char destBuffer[20]);

    /**
     * check if password is correct by password hash
     * @param passHashBuffer password hash buffer contains the hash value to be check
     * @param bufferSize hash buffer size
     * @return true if the hash value is correct.
     */
    bool checkPassword(const unsigned char *passHashBuffer, size_t bufferSize);

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

    /**
     * Server side load file into `fileContent` in bytes format.
     * @param filename file name with content
     */
    void loadFile(const char* filename);

    /**
     * Client side could write `fileContent` into a target file
     * @param filenmae target file name
     */
    void writeFile(const char* filename) const;

    /**
     * Set password with a char pointer buffer and its length.
     * @param password password char pointer buffer
     * @param passwdLen password buffer length
     */
    void setPassword(const char* password, size_t passwdLen);

    /**
     * Waiting for the first handshake packets. Server side socket should call this function first.
     * Override parent `startListen` for password validation.
     */
    void startListen() override;

    /**
     * Waiting for the client password request, authenticate it.
     */
    void startAuthenticate();

    /**
     * Client side socket should call this function bind its peer address and port,
     *     and send the first handshake packet.
     * Override parent `connectForeignAddressPort` for password validation.
     * @param address Foreign peer address
     * @param port Foreign peer port
     */
    void connectForeignAddressPort (const string& address, unsigned short port) override;

    bool sendAuthenticate();

    //passCat = 3 * password, connect with ','
    int connectToServer(const char* passCat, int passLen, const char* add);

    int connectToClient(const char* pass, int passLen, const char* add);

    void recvPacket(char* target, unsigned short& type, unsigned int& length, unsigned int& pID, char* content);

    bool checkPassword(const char* pw3, int len3, const char* pw, int len);

    char* sha1(char* src);

protected:
    /**
     * Indicate the current password try times both in client socket and server socket.
     */
    int passwdTryTimes = 0;

    /**
     * Once password reach the max try times,
     *  the server with disconnect from the client (send TERMINATE packet).
     */
    int passwdMaxTryTimes = 3;

    /**
     * The file content to be transmitted, which is stored in bytes format.
     *  Server side should load content via `loadFile`; Client received from peer side.
     * `ContentLength` contains the length of `fileContent`.
     */
    char *fileContent = nullptr;
    unsigned int contentLength = 0;

    /**
     * The password bind to the socket.
     * Server side validate password with this; Client side send this in PASS_RESP
     */
    char *bindPassword = nullptr;

    /**
     * Used when hash value is calculate.
     */
    string hashSalt = "SOME_SECRET";
};


#endif //TLSUDPPROTOCOL_APPSOCKET_H
