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
#include <json/value.h>

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
    formatPacket getMsgAckPacket(unsigned short seqNumber) const;
    formatPacket getLenAckPacket() const;
    formatPacket getMsgPacket(unsigned short seqNumber) const;
    formatPacket getFinPacket(bool isMsgFin = true) const;

protected:
    /**
     * These construct functions can only be called from children class
     */
    ReliableSocket(): UdpSocket(){}
    explicit ReliableSocket(unsigned short localPort) : UdpSocket(localPort) {}
    ReliableSocket(const string &localAddress, unsigned short localPort) : UdpSocket(localAddress, localPort) {}

public:
    /**
     *   Construct a reliable UDP socket
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    explicit ReliableSocket(const char *configPath);

    /**
     *   Construct a reliable UDP socket with the given local port
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    ReliableSocket(unsigned short localPort, const char *configPath);

    /**
     *   Construct a reliable UDP socket with the given local port and address
     *   @param localAddress local address
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    ReliableSocket(const string &localAddress, unsigned short localPort, const char *configPath);

    /**
     * Release the memory allocated in packetsBuffer.
     */
    ~ReliableSocket();

    /**
     * Override the parent function using the exactly the same function
     *     cause protected inherit can't access parent function from outer.
     */
    string getForeignAddress() const override {return UdpSocket::getForeignAddress();}
    unsigned short getForeignPort() const override {return UdpSocket::getForeignPort();}

    /**
     * Load config from a json file. Such as: timeoutInterval, packetSize.
     * @param configPath configuration file path.
     */
    virtual Json::Value loadConfig(const char *configPath);

    /**
     * Set packetsBuffer with char pointer and buffer length, avoid terminated char
     * @param message message char pointer
     * @param mLength message buffer length
     */
    void setPackets(const char *message, unsigned int mLength);

    /**
     * Set packetsBuffer with message body. You should call sendPackets manually after setting
     * @param messageBody the message body to be sent.
     */
    void setPackets(const string& messageBody);

    /**
     * Accept integer as parameter. It will malloc empty space with message length.
     * @param messageLength message length
     */
    void setPackets(unsigned int mLength);

    /**
     * Get message length of the combined packets
     * @return message length
     */
    unsigned int getMessageLength() const;

    /**
     * Combine seperated packets into a char array,
     *  with the help of dest length, original message can be
     */
    void readMessage(char *destBuffer, int destBufferSize) const;

    /**
     * Server side socket should call this function bind its address and port first
     * @exception you shouldn't call this function when you have already bind once
     */
     void bindLocalAddressPort (const string& address, unsigned short port);

     /**
      * Waiting for the first handshake packets.
      * Server side socket should call this function first.
      */
     virtual void startListen();

     /**
      * Client side socket should call this function bind its peer address and port,
      *     and send the first handshake packet.
      * @param address Foreign peer address
      * @param port Foreign peer port
      */
     virtual void connectForeignAddressPort (const string& address, unsigned short port);

     /**
      * Major function of this socket, reliably receive message from peer side
      */
     virtual void receiveMessage();

     /**
      * Major function of this socket, reliably send message to peer side.
      */
     virtual void sendMessage();

private:
    void sendSinglePacket(unsigned short seqNumber);
    void sendSinglePacket(formatPacket fpk, bool &successCheck);

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
    unsigned int messageLength = 0;

    /**
     * A list of mutexs created for each packet
     *  - Server side: Used for sending ack packet back;
     *  - Client side: Used for waiting ack and resending.
     */
    vector<mutex> packetsMutex;
};


#endif //TLSUDPPROTOCOL_RELIABLESOCKET_H
