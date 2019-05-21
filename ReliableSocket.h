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
class ReliableSocket: public UdpSocket {
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
     *   Construct a UDP socket with the given local port and address
     *   @param localAddress local address
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create UDP socket
     */
    ReliableSocket(const string &localAddress, unsigned short localPort,
                   const char *configPath = "./config.json") throw(SocketException);

    ~ReliableSocket();

    /**
     * Load config from a json file. timeoutInterval, packetSize.
     * @param configPath configuration file path.
     */
    void loadConfig(const char *configPath);

    /**
     * Set packetsBuffer with message body. You should call handShake manually after setting
     * @param messageBody the message body to be sent.
     */
    void setPackets(const string& messageBody);

    void startListen();

protected:
    condition_variable completeFlag;    // variable indicate complete

    /**
     *  Resend timeout duration.
     *  Integer represent, take milliseconds as unit.
     */
    chrono::milliseconds timeoutInterval = 0ms;

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

    /**
     * A list of mutexs created for each packet
     *  - Server side: Used for sending ack packet back;
     *  - Client side: Used for waiting ack and resending.
     */
    vector<mutex> packetsMutex;
};


#endif //TLSUDPPROTOCOL_RELIABLESOCKET_H
