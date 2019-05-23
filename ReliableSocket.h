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
    formatPacket getAckPacket(unsigned short seqNumber, bool isHandShake = false) const throw(SocketException);
    formatPacket getMsgPacket(unsigned short seqNumber) const throw(SocketException);
    formatPacket getFinPacket(bool isMsgFin) const;

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

    char *readMessage() const;

    /**
     * Set the peer address and port, leave default indicate any address
     * @param pAddress peer IP address
     * @param pPort peer port
     */
    void setPeer(const string& pAddress, unsigned short pPort);

    string getPeerAddress() const {return peerAddress;}

    unsigned short getPeerPort() const {return peerPort;}

    void receiveMessage() throw(SocketException);

    void sendMessage() throw(SocketException);

private:
    void sendSinglePacket(unsigned short seqNumber) throw(SocketException);
    void sendSinglePacket(formatPacket fpk, bool &successCheck) throw(SocketException);

protected:
    condition_variable completeCond;    // variable indicate complete
    mutex completeMutex;

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

    /**
     * Peer address and peer port.
     *  when the socket serve as a server, empty indicate any where
     */
    string peerAddress;
    unsigned short peerPort = 0;
};


#endif //TLSUDPPROTOCOL_RELIABLESOCKET_H
