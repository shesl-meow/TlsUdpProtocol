//
// Created by shesl-meow on 19-5-21.
//

#ifndef TLSUDPPROTOCOL_RELIABLESOCKET_H
#define TLSUDPPROTOCOL_RELIABLESOCKET_H

#include "./UdpSocket.h"

#include <chrono>               // chrono::milliseconds timeoutInterval
#include <vector>               // vector<char *> packetBuffer & vector<mutex> packetsMutex
#include <condition_variable>   // condition_variable completeFlag;
#include <thread>               // vector<mutex> packetsMutex

using namespace std::chrono_literals;   //  0ms, 1s

/**
  *   Implement a reliable socket from UDP socket
  */
class ReliableSocket: protected UdpSocket {

private:
    /**
     * Reliable socket format, with three header.
     * More specific description about the bits message was list on README.md
     */
    struct formatPacket {
        unsigned short bodySize = 0;
        unsigned short seqNumber = 0;
        unsigned short flag = 0;
        char *packetBody = nullptr;
    };

    /**
     * Transfer a char array to a formatted packet struct, remember release memory.
     * @param packet a char array from the peer side
     * @return a formatted packet struct
     */
    static formatPacket parsePacket(char *packet);

    /**
     * Trasfer a formatted packet struct to a char array, remember release memory.
     * @param fpacket a formatted packet
     * @return a char array to send to peer side
     */
    static char * deparsePacket(formatPacket fpacket);

    /**
     * Four function that generate a formatted packet struct
     * @return a formatted packet struct
     */
    formatPacket getHanPacket() const;
    formatPacket getLenPacket() const;
    formatPacket getMsgAckPacket(unsigned short seqNumber) const throw(SocketException);
    formatPacket getLenAckPacket() const throw(SocketException);
    formatPacket getMsgPacket(unsigned short seqNumber) const throw(SocketException);
    formatPacket getFinPacket(bool isMsgFin = true) const;

public:
    /**
     *   Construct a reliable UDP socket
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    ReliableSocket(const char *configPath = "./config.json") throw(SocketException);

    /**
     *   Construct a reliable UDP socket with the given local port
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    ReliableSocket(unsigned short localPort, const char *configPath = "./config.json")
    throw(SocketException);

    /**
     *   Construct a reliable UDP socket with the given local port and address
     *   @param localAddress local address
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    ReliableSocket(const string &localAddress, unsigned short localPort,
                   const char *configPath = "./config.json") throw(SocketException);

    /**
     * Release the memory allocated in packetsBuffer.
     */
    ~ReliableSocket();

    /**
     * Load config from a json file. Such as: timeoutInterval, packetSize.
     * @param configPath configuration file path.
     */
    void loadConfig(const char *configPath);

    /**
     * Set packetsBuffer with message body. You should call sendPackets manually after setting
     * @param messageBody the message body to be sent.
     */
    void setPackets(const string& messageBody) throw(SocketException);

    /**
     * Accept integer as parameter. It will malloc empty space with message length.
     * @param messageLength message length
     */
    void setPackets(unsigned int mLength) throw(SocketException);

    /**
     * Get message length of the combined packets
     * @return message length
     */
    unsigned int getMessageLength() const;

    /**
     * Combine seperated packets into a char array,
     *  with the help of dest length, original message can be
     */
    void readMessage(char *destBuffer, int destBufferSize) const throw(SocketException);

    /**
     * Server side socket should call this function bind its address and port first
     * @exception you shouldn't call this function when you have already bind once
     */
     void bindLocalAddressPort (const string& address, unsigned short port) throw(SocketException);

     /**
      * Waiting for the first handshake packets.
      * Server side socket should call this function first.
      */
     void startListen();

     /**
      * Client side socket should call this function bind its peer address and port,
      *     and send the first handshake packet.
      * @param address Foreign peer address
      * @param port Foreign peer port
      */
     void connectForeignAddressPort (const string& address, unsigned short port) throw(SocketException);

    string getForeignAddress() const {return UdpSocket::getForeignAddress();}

    unsigned short getForeignPort() const {return UdpSocket::getForeignPort();}

    void receiveMessage() throw(SocketException);

    void sendMessage() throw(SocketException);

private:
    void sendSinglePacket(unsigned short seqNumber) throw(SocketException);
    void sendSinglePacket(formatPacket fpk, bool &successCheck) throw(SocketException);

protected:
    /**
     *  Resend timeout duration.
     *  Integer represent, take milliseconds as unit.
     */
    chrono::milliseconds timeoutInterval = 0ms;

    /**
     * Once program reach a timeoutInterval, retry will be triggered.
     * retryTimes indicate the maximum times program try to retry.
     */
    unsigned int retryTimes = 0;

    /**
     * Buffer size when receive message from the peer side.
     */
    unsigned short bufferSize = 0;

    /**
     *  Packet size of this reliable protocol.
     *  Maximum packet size 65535 bytes = 64 kbytes
     */
    unsigned short packetSize = 0;

    /**
     * A list of buffer contains all packets whose length is packetSize.
     *  - Server side: Received packets;
     *  - Client side: Packets for sending.
     */
    vector<char *> packetsBuffer;
    vector<bool> packetsConfirm;
    unsigned int messageLength;

    /**
     * A list of mutexs created for each packet
     *  - Server side: Used for sending ack packet back;
     *  - Client side: Used for waiting ack and resending.
     */
    vector<mutex> packetsMutex;
};


#endif //TLSUDPPROTOCOL_RELIABLESOCKET_H
