//
// Created by shesl-meow on 19-5-21.
//

#include "ReliableSocket.h"

#include <iostream>         // cerr
#include <cassert>
#include <string>
#include <cstring>          // memcpy()
#include <fstream>          // ifstream configFile
#include <streambuf>        // istreambuf_iterator
#include <sstream>          // convert string to int
#include <thread>
#include "json/json.h"      // Parse config string

/**
 *  Const value for parsing flag.
 *  The program parses flag as little-endian `unsigned short`
 */
#define HAN_FLAG 0x80u
#define FIN_FLAG 0x40u
#define ACK_FLAG 0x20u
#define MSG_FLAG 0x10u

#define RELIABLE_DEBUG true

// ReliableSocket Code

ReliableSocket::formatPacket ReliableSocket::parsePacket(char *packet) {
    formatPacket fpacket;
    fpacket.bodySize = *(unsigned short *)packet;
    fpacket.seqNumber = *((unsigned short *)packet + 1);
    fpacket.flag = *((unsigned short *)packet + 2);
    if (fpacket.bodySize > 0) {
        fpacket.packetBody = new char(fpacket.bodySize - 6);
        memcpy(fpacket.packetBody, packet + 6, fpacket.bodySize);
    } else fpacket.packetBody = nullptr;
    return fpacket;
}

char* ReliableSocket::deparsePacket(ReliableSocket::formatPacket fpacket) {
    char *packet = new char(6 + fpacket.bodySize);
    *((unsigned short *)packet) = fpacket.bodySize;
    *((unsigned short *)packet + 1) = fpacket.seqNumber;
    *((unsigned short *)packet + 2) = fpacket.flag;
    memcpy(packet + 6, fpacket.packetBody, fpacket.bodySize);
    return packet;
}

ReliableSocket::formatPacket ReliableSocket::getHanPacket() const {
    string mLength = to_string(messageLength);
    formatPacket hpacket;
    hpacket.bodySize = mLength.size();
    hpacket.flag = HAN_FLAG;
    hpacket.packetBody = new char(hpacket.bodySize);
    memcpy(hpacket.packetBody, mLength.c_str(), mLength.size());
    return hpacket;
}

ReliableSocket::formatPacket ReliableSocket::getAckPacket(unsigned short seqNumber,
        bool isHandShake) const throw(SocketException){
    if(seqNumber * packetSize > messageLength)
        throw SocketException("Sequence number out of range", false);
    formatPacket apacket;
    apacket.bodySize = 0;
    apacket.seqNumber = seqNumber;
    apacket.flag = (isHandShake) ? (HAN_FLAG | ACK_FLAG) : ACK_FLAG;
    return apacket;
}

ReliableSocket::formatPacket ReliableSocket::getMsgPacket(unsigned short seqNumber)
const throw(SocketException){
    if(seqNumber * packetSize > messageLength)
        throw SocketException("Sequence number out of range", false);
    formatPacket mpacket;
    mpacket.seqNumber = seqNumber;
    mpacket.flag = MSG_FLAG;
    if( (seqNumber + 1) * packetSize + (messageLength % packetSize) > messageLength) {
        mpacket.bodySize = messageLength % seqNumber;
        mpacket.packetBody = new char(mpacket.bodySize);
        memcpy(mpacket.packetBody, packetsBuffer.at(seqNumber), mpacket.bodySize);
    } else {
        mpacket.bodySize = packetSize;
        mpacket.packetBody = new char(mpacket.bodySize);
        memcpy(mpacket.packetBody, packetsBuffer.at(seqNumber), mpacket.bodySize);
    }
    return mpacket;
}

ReliableSocket::formatPacket ReliableSocket::getFinPacket(bool isMsgFin) const {
    formatPacket fnpacket;
    fnpacket.flag = isMsgFin ? (FIN_FLAG | MSG_FLAG) : (FIN_FLAG | ACK_FLAG);
    return fnpacket;
}


ReliableSocket::ReliableSocket(const char *configPath) throw(SocketException) : UdpSocket(){
    loadConfig(configPath);
}

ReliableSocket::ReliableSocket(unsigned short localPort, const char *configPath)
throw(SocketException): UdpSocket(localPort) {
    loadConfig(configPath);
}

ReliableSocket::ReliableSocket(const string &localAddress, unsigned short localPort,
        const char *configPath) throw(SocketException): UdpSocket(localAddress, localPort) {
    loadConfig(configPath);
}

ReliableSocket::~ReliableSocket() {
    for(auto packet: packetsBuffer) delete []packet;
}

void ReliableSocket::loadConfig(const char *configPath) {
    // TODO: Load all chars into string from file.
    ifstream configFile;
    configFile.open(configPath);

    if (configFile.is_open()) {
        string configStr((istreambuf_iterator<char>(configFile)), istreambuf_iterator<char>());

        /** TODO: Parse json string, from StackOverflow:
         *    https://stackoverflow.com/questions/47283908/parsing-json-string-with-jsoncpp
         */
        Json::CharReaderBuilder configBuilder;
        Json::CharReader *configReader = configBuilder.newCharReader();
        Json::Value configValue;

        string parsingErrors;
        if (!(configReader->parse(configStr.c_str(), configStr.c_str() + configStr.size(),
                &configValue, &parsingErrors))) {
            cerr << "Parsing " << configPath << " error: " << parsingErrors << endl;
        } else {
            timeoutInterval = chrono::milliseconds(configValue["timeoutInterval"].asInt());
            packetSize = configValue["packetSize"].asInt();
            bufferSize = configValue["bufferSize"].asInt();
            retryTimes = configValue["retryTimes"].asInt();
        }
        configFile.close();
    } else {
        cerr << "Can't open config file " << configPath << endl;
    }
    if (timeoutInterval == 0ms) timeoutInterval = 1s;
    if (packetSize == 0) packetSize = 1024;
    if (bufferSize == 0) bufferSize = 255;
    if (retryTimes == 0) retryTimes = 3;
}

void ReliableSocket::setPackets(const string &messageBody) throw(SocketException) {
    if (messageBody.size() >= UINT32_MAX) throw SocketException("Message to long", false);

    if (messageLength != 0) {
        for(auto packet: packetsBuffer) delete []packet;
        packetsBuffer.clear(); packetsConfirm.clear();
        messageLength = 0;
    }

    messageLength = messageBody.size();
    for(int i = 0; i < messageLength / packetSize; ++i){
        char *packet = new char[packetSize];
        memcpy(packet, messageBody.substr(i * packetSize, packetSize).c_str(), packetSize);
        packetsBuffer.push_back(packet); packetsConfirm.push_back(false);
    }
    if(messageLength % packetSize != 0){
        char *packet = new char[packetSize];
        memcpy(packet, messageBody.substr(messageLength - messageLength % packetSize).c_str(), packetSize);
        packetsBuffer.push_back(packet); packetsConfirm.push_back(false);
    }
}

char* ReliableSocket::readMessage() const {
    char *message = new char(messageLength + 1);
    for(int i = 0; i < messageLength / packetSize; ++i) {
        memcpy(message + i*packetSize, packetsBuffer.at(i), packetSize);
    }
    if (messageLength % packetSize != 0) {
        memcpy(message + messageLength - messageLength % packetSize,
                packetsBuffer.at(messageLength / packetSize), messageLength % packetSize);
    }
    message[messageLength] = '\0';
    return message;
}

void ReliableSocket::setPeer(const string &pAddress, unsigned short pPort) {
    peerAddress = pAddress, peerPort = pPort;
}

void ReliableSocket::receiveMessage() throw(SocketException) {
    // TODO: STEP1 -- listen handshake packet
    char *receiveBuffer = new char(bufferSize);
    int receiveSize = 0;
    string sourceAddress;
    unsigned short sourcePort = 0;
    while (true) {
        receiveSize = recvFrom(receiveBuffer, bufferSize, sourceAddress, sourcePort);
        auto fpacket = parsePacket(receiveBuffer);
        if (
            ((fpacket.flag ^ HAN_FLAG) != 0u) ||
            ((!peerAddress.empty()) && (peerAddress != sourceAddress)) ||
            ((peerPort != 0) && (peerPort != sourcePort))
        ){ delete []fpacket.packetBody; continue;}

        peerAddress = sourceAddress, peerPort = sourcePort;
        char *endBody = fpacket.packetBody + fpacket.bodySize;
        auto mLength = strtol(fpacket.packetBody, &endBody, 10);

        // TODO: Allocate buffer size using default char ' '
        string paddingStr(mLength, ' ');
        setPackets(paddingStr);
        delete []fpacket.packetBody; break;
    }

    // TODO: STEP2 -- sending handshake ack packet back
    auto hpacket = getAckPacket(0, true);
    char *packet = deparsePacket(hpacket);
    bool hanAckSuccess = false;
    auto t = thread([this, hpacket, &hanAckSuccess]{
        this->sendSinglePacket(hpacket, hanAckSuccess);
    });

    // TODO: STEP3 -- waiting for all packets and sending acks
    while (true) {
        receiveSize = recvFrom(receiveBuffer, bufferSize, sourceAddress, sourcePort);
        auto fpacket = parsePacket(receiveBuffer);
        if (
            ((fpacket.flag ^ MSG_FLAG) != 0u) ||
            (peerAddress != sourceAddress) || (peerPort != sourcePort)
        ) { delete []fpacket.packetBody; continue; }

        // TODO: save packet and send ack
        packetsConfirm.at(fpacket.seqNumber) = true; hanAckSuccess = true;
        memcpy(packetsBuffer.at(fpacket.seqNumber), fpacket.packetBody, fpacket.bodySize);
        auto apacket = getAckPacket(fpacket.seqNumber, false);
        auto ackpacket = deparsePacket(apacket);
        sendTo(ackpacket, apacket.bodySize + 6, peerAddress, peerPort);
        delete []ackpacket; delete []apacket.packetBody;

        bool compeleteFlag = true;
        for(auto confirm: packetsConfirm) if(!confirm) compeleteFlag = false;
        delete []fpacket.packetBody;
        if (compeleteFlag) {break;}
    }

    // TODO: STEP4 -- sending FIN packet
    auto fnpacket = getFinPacket(false);
    auto finpacket = deparsePacket(fnpacket);
    sendTo(finpacket, fnpacket.bodySize + 6, peerAddress, peerPort);
    delete []fnpacket.packetBody; delete []finpacket;

    delete []packet; delete []receiveBuffer;
    t.join();
    delete []hpacket.packetBody; delete []fnpacket.packetBody;
}

void ReliableSocket::sendMessage() throw(SocketException) {
    // TODO: STEP1 -- send handshake packet
    auto hpacket = getHanPacket();
    char *packet = deparsePacket(hpacket);
    bool handshakeSuccess = false;
    auto t = thread([this, hpacket, &handshakeSuccess]{
        this->sendSinglePacket(hpacket, handshakeSuccess);
    });

    // TODO: STEP2 -- waiting for handshake ack.
    char *receiveBuffer = new char(bufferSize);
    int receiveSize = 0;
    string sourceAddress;
    unsigned short sourcePort = 0;
    while (true) {
        receiveSize = recvFrom(receiveBuffer, bufferSize, sourceAddress, sourcePort);
        auto fpacket = parsePacket(receiveBuffer);
        delete []fpacket.packetBody;
        if (((fpacket.flag ^ (HAN_FLAG | ACK_FLAG)) == 0u) &&
            (peerAddress == sourceAddress) && (peerPort == sourcePort)) break;
    }
    handshakeSuccess = true;
    delete []packet; delete []hpacket.packetBody;

    // TODO: STEP3 -- sending all packets
    vector<thread> senderThreads;
    for(int i = 0; i < packetsBuffer.size(); ++i) {
        thread tt([this, i]{this->sendSinglePacket(i);});
        senderThreads.push_back( move(tt) );
    }

    // TODO: STEP4 -- waiting for all packets' ack
    while (true) {
        receiveSize = recvFrom(receiveBuffer, bufferSize, sourceAddress, sourcePort);
        auto fpacket = parsePacket(receiveBuffer);
        if ((fpacket.flag ^ (FIN_FLAG | ACK_FLAG)) == 0u) {
            for (auto confirm: packetsConfirm) confirm = true;
            delete []fpacket.packetBody;
            break;
        } else if ( ((fpacket.flag ^ (ACK_FLAG)) == 0u) && (peerAddress == sourceAddress) && (peerPort == sourcePort) ){
            packetsConfirm.at(fpacket.seqNumber) = true;
            bool completeFlag = true;
            for(auto confirm: packetsConfirm) if(!confirm) completeFlag = false;
            delete []fpacket.packetBody;
            if (completeFlag) break;
        } else {
            delete []fpacket.packetBody;
            continue;
        }
    }
    t.join();
    for (auto &tt: senderThreads) tt.join();

    delete []receiveBuffer;
}

void ReliableSocket::sendSinglePacket(unsigned short seqNumber) throw(SocketException) {
    auto fpk = getMsgPacket(seqNumber);
    char *packet = deparsePacket(fpk);
    sendTo(packet, fpk.bodySize + 6, peerAddress, peerPort);
    for (unsigned int i = 0; i < retryTimes; ++i) {
        this_thread::sleep_for(timeoutInterval);
        if (packetsConfirm.at(seqNumber)){
            delete []packet; delete []fpk.packetBody; return;
        } else sendTo(packet, fpk.bodySize + 6, peerAddress, peerPort);
    }
    delete [] packet; delete []fpk.packetBody;
    throw SocketException("Lose connection. Seq: " + to_string(fpk.seqNumber), true);
}

void ReliableSocket::sendSinglePacket(ReliableSocket::formatPacket fpk, bool &successCheck)
throw(SocketException) {
    char *packet = deparsePacket(fpk);
    sendTo(packet, fpk.bodySize + 6, peerAddress, peerPort);
    for (unsigned int i = 0; i < retryTimes; ++i) {
        this_thread::sleep_for(timeoutInterval);
        if (successCheck) {
            delete [] packet;
            return;
        } else sendTo(packet, fpk.bodySize + 6, peerAddress, peerPort);
    }
    delete [] packet;
    throw SocketException("Lose connection. Flag: " + to_string(fpk.flag), true);
}