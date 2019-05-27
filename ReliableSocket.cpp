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
#include "sys/socket.h"     // setsockopt()

/**
 *  Const value for parsing flag.
 *  The program parses flag as little-endian `unsigned short`
 */
#define HAN_FLAG 0x80u
#define LEN_FLAG 0x40u
#define FIN_FLAG 0x20u
#define ACK_FLAG 0x10u
#define MSG_FLAG 0x8u

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
    formatPacket hpacket;
    hpacket.flag = HAN_FLAG;
    return hpacket;
}

ReliableSocket::formatPacket ReliableSocket::getLenPacket() const {
    formatPacket lpacket;
    lpacket.flag = LEN_FLAG;
    string mLength = to_string(messageLength);
    lpacket.bodySize = mLength.size();
    lpacket.packetBody = new char(lpacket.bodySize);
    memcpy(lpacket.packetBody, mLength.c_str(), mLength.size());
    return lpacket;
}

ReliableSocket::formatPacket ReliableSocket::getMsgAckPacket(unsigned short seqNumber) const throw(SocketException){
    if(seqNumber * packetSize > messageLength)
        throw SocketException("Sequence number out of range", false);
    formatPacket mapacket;
    mapacket.seqNumber = seqNumber;
    mapacket.flag = (MSG_FLAG | ACK_FLAG);
    return mapacket;
}

ReliableSocket::formatPacket ReliableSocket::getLenAckPacket() const throw(SocketException) {
    formatPacket lapacket;
    lapacket.flag = (LEN_FLAG | ACK_FLAG);
    return lapacket;
}

ReliableSocket::formatPacket ReliableSocket::getMsgPacket(unsigned short seqNumber)
const throw(SocketException){
    if(seqNumber * packetSize > messageLength)
        throw SocketException("Sequence number out of range", false);
    formatPacket mpacket;
    mpacket.seqNumber = seqNumber;
    mpacket.flag = MSG_FLAG;
    if( (seqNumber + 1) * packetSize + (messageLength % packetSize) > messageLength) {
        mpacket.bodySize = messageLength % packetSize;
        mpacket.packetBody = new char (mpacket.bodySize);
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

    // TODO: apply timout interval config
#ifdef WIN32
    DWORD timeout = timeoutInterval.count();
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval timeoutStruct = {0, 0};
    timeoutStruct.tv_sec = timeoutInterval.count() / 1000;
    timeoutStruct.tv_usec = (timeoutInterval.count() % 1000) * 1000;
    setsockopt(sockDesc, SOL_SOCKET, SO_RCVTIMEO, &timeoutStruct, sizeof(timeoutStruct));
#endif
#ifdef RELIABLE_DEBUG
    cout << "Set timeout: " << timeoutInterval.count() << "ms" << endl;
#endif
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
#ifdef RELIABLE_DEBUG
    cout << "Set packet: " << messageBody << endl;
#endif
}

void ReliableSocket::setPackets(unsigned int mLength) throw(SocketException) {
    if (messageLength != 0) {
        for(auto packet: packetsBuffer) delete []packet;
        packetsBuffer.clear(); packetsConfirm.clear();
        messageLength = 0;
    }

    messageLength = mLength;
    for(int i = 0; i < messageLength / packetSize; ++i){
        char *packet = new char (packetSize);
        packetsBuffer.push_back(packet); packetsConfirm.push_back(false);
    }
    if(messageLength % packetSize != 0){
        char *packet = new char[packetSize];
        packetsBuffer.push_back(packet); packetsConfirm.push_back(false);
    }
#ifdef RELIABLE_DEBUG
    cout << "Set empty packet with length=" << mLength << endl;
#endif
}

unsigned int ReliableSocket::getMessageLength() const {return messageLength;}

void ReliableSocket::readMessage(char *destBuffer, int destBufferSize) const throw(SocketException) {
    if (destBufferSize <= messageLength) {
        throw SocketException("Please passed a buffer with more space.");
    }
    for(int i = 0; i < messageLength / packetSize; ++i) {
        memcpy(destBuffer + i*packetSize, packetsBuffer.at(i), packetSize);
    }
    if (messageLength % packetSize != 0) {
        memcpy(destBuffer + messageLength - messageLength % packetSize,
                packetsBuffer.at(messageLength / packetSize), messageLength % packetSize);
    }
    destBuffer[messageLength] = '\0';
}

void ReliableSocket::bindLocalAddressPort(const string &address, unsigned short port)
throw(SocketException) {
    this->setLocalAddressAndPort(address, port);
#ifdef RELIABLE_DEBUG
    cout << "Bind local: " << address << ":" << port << endl;
#endif
}

void ReliableSocket::startListen() {
    char *receiveBuffer = new char(bufferSize);
    unsigned int receiveSize = 0;
    string sourceAddress;
    unsigned short sourcePort;
#ifdef RELIABLE_DEBUG
    cout << "Start listening." << flush;
#endif
    // TODO: receive first handshake packet from peer side.
    while (true) {
        receiveSize = recvFrom(receiveBuffer, bufferSize, sourceAddress, sourcePort);
        if (receiveSize == -1){
#ifdef RELIABLE_DEBUG
            cout << "." << flush;
#endif
            continue;
        }
        auto fpacket = parsePacket(receiveBuffer);
        delete fpacket.packetBody;
        if ( (fpacket.flag ^ HAN_FLAG) == 0u) break;
    }
    connect(sourceAddress, sourcePort);
    delete receiveBuffer;
#ifdef RELIABLE_DEBUG
    cout << endl << "Connect to " << getForeignAddress() << ":" << getForeignPort() << endl;
#endif

    // TODO: send back ack of first handshake packet
    auto hpacket = getHanPacket();
    char *packet = deparsePacket(hpacket);
    this->send(packet, hpacket.bodySize + 6);
    delete packet;
    delete hpacket.packetBody;
}

void ReliableSocket::connectForeignAddressPort(const string &address, unsigned short port)
throw(SocketException) {
    // TODO: set default send target, then send handshake packet
    this->connect(address, port);
    auto hpacket = getHanPacket();
    bool connectSuccess = false;
    auto t = thread([this, hpacket, &connectSuccess]{this->sendSinglePacket(hpacket, connectSuccess);});

    // TODO: receive first handshake packet ack.
    char *receiveBuffer = new char(bufferSize);
    unsigned int receiveSize;
    for(unsigned int i = 0; i < retryTimes; i++) {
        receiveSize = this->recv(receiveBuffer, bufferSize);
        if (receiveSize == -1) continue;
        auto fpacket = parsePacket(receiveBuffer);
        delete fpacket.packetBody;
        if ( ((fpacket.flag ^ HAN_FLAG) == 0u) && fpacket.bodySize == 0u ) break;
        else {
#ifdef RELIABLE_DEBUG
            cout << "[Receiving Handshake ACK] Drop packets: " << string(fpacket.packetBody, fpacket.bodySize) << endl;
#endif
        }
    }
    connectSuccess = true;
    t.join();
    delete hpacket.packetBody;
}

void ReliableSocket::receiveMessage() throw(SocketException) {
    // TODO: STEP1 -- receive length packet
    char *receiveBuffer = new char(bufferSize);
    int receiveSize = 0;
    while (true) {
        receiveSize = recv(receiveBuffer, bufferSize);
        if (receiveSize < 0) continue;
        auto fpacket = parsePacket(receiveBuffer);
        if ( (fpacket.flag ^ LEN_FLAG) != 0u ){
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Length] Drop packets: " << string(fpacket.packetBody, fpacket.bodySize) << endl;
        #endif
            delete fpacket.packetBody;
            continue;
        } else {
            char *endBody = fpacket.packetBody + fpacket.bodySize;
            auto mLength = strtol(fpacket.packetBody, &endBody, 10);
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Length] Ready receive length=" << mLength << endl;
        #endif
            setPackets(mLength);
            delete fpacket.packetBody; break;
        }
    }

    // TODO: STEP2 -- sending length ack packet back
    auto hpacket = getLenAckPacket();
    bool lenAckSuccess = false;
    thread t ([this, hpacket, &lenAckSuccess]{this->sendSinglePacket(hpacket, lenAckSuccess);});

    // TODO: STEP3 -- waiting for all packets and sending acks
    while (true) {
        receiveSize = recv(receiveBuffer, bufferSize);
        if (receiveSize < 0) continue;
        auto fpacket = parsePacket(receiveBuffer);
        if ( (fpacket.flag ^ MSG_FLAG) == 0u) {
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Packets] [" << fpacket.seqNumber << "] "
                << string(fpacket.packetBody, fpacket.bodySize) << endl;
            cout << "[Send ACK packet] [" << fpacket.seqNumber << "] "
                << string(fpacket.packetBody, fpacket.bodySize) << endl;
        #endif
            packetsConfirm.at(fpacket.seqNumber) = true; lenAckSuccess = true;
            // TODO: save packet and send ack
            memcpy(packetsBuffer.at(fpacket.seqNumber), fpacket.packetBody, fpacket.bodySize);
            auto mapacket = getMsgAckPacket(fpacket.seqNumber);
            auto ackpacket = deparsePacket(mapacket);
            this->send(ackpacket, mapacket.bodySize + 6);
            delete ackpacket; delete mapacket.packetBody;
        } else {
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Packets] Drop packet " <<
                string(fpacket.packetBody, fpacket.bodySize) << endl;
        #endif
            delete []fpacket.packetBody; continue;
        }

        bool compeleteFlag = true;
        for(auto confirm: packetsConfirm) if(!confirm) compeleteFlag = false;
        delete []fpacket.packetBody;
        if (compeleteFlag) {break;}
    }

    delete []receiveBuffer;
    t.join();
    delete []hpacket.packetBody;
}

void ReliableSocket::sendMessage() throw(SocketException) {
    // TODO: STEP1 -- send length packet
    auto lpacket = getLenPacket();
    bool lenSuccess = false;
    thread t ([this, lpacket, &lenSuccess]{this->sendSinglePacket(lpacket, lenSuccess);});

    // TODO: STEP2 -- waiting for handshake ack.
    char *receiveBuffer = new char(bufferSize);
    int receiveSize = 0;
    while (true) {
        receiveSize = recv(receiveBuffer, bufferSize);
        if (receiveSize < 0) continue;
        auto fpacket = parsePacket(receiveBuffer);
        if ((fpacket.flag ^ (LEN_FLAG | ACK_FLAG)) == 0u ){
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Length ACK] Received!" << endl;
        #endif
            lenSuccess = true;
            delete fpacket.packetBody; break;
        } else {
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Length ACK] Drop packet " << string(fpacket.packetBody, fpacket.bodySize) << endl;
        #endif
            delete fpacket.packetBody;
        }
    }
    delete lpacket.packetBody;

    // TODO: STEP3 -- sending all packets
    vector<thread> senderThreads;
    for(int i = 0; i < packetsBuffer.size(); ++i) {
        thread tt([this, i]{this->sendSinglePacket(i);});
        senderThreads.push_back( move(tt) );
    }

    // TODO: STEP4 -- waiting for all packets' ack
    while (true) {
        receiveSize = recv(receiveBuffer, bufferSize);
        if (receiveSize < 0) continue;
        auto fpacket = parsePacket(receiveBuffer);
        delete fpacket.packetBody;
        if ( (fpacket.flag ^ (MSG_FLAG | ACK_FLAG)) == 0u) {
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Packets ACK] received [" << fpacket.seqNumber << "]" << endl;
        #endif
            packetsConfirm.at(fpacket.seqNumber) = true;
        } else {
        #ifdef RELIABLE_DEBUG
            cout << "[Receiving Packets ACK] Drop packets " <<
                string(fpacket.packetBody, fpacket.bodySize) << endl;
        #endif
            continue;
        }

        bool completeFlag = true;
        for(auto confirm: packetsConfirm) if(!confirm) completeFlag = false;
        if (completeFlag) break;
    }
    t.join();
    for (auto &tt: senderThreads) tt.join();

    delete []receiveBuffer;
}

void ReliableSocket::sendSinglePacket(unsigned short seqNumber) throw(SocketException) {
    auto fpk = getMsgPacket(seqNumber);
    char *packet = deparsePacket(fpk);
    for (unsigned int i = 0; i < retryTimes; ++i) {
        this->send(packet, fpk.bodySize + 6);
    #ifdef RELIABLE_DEBUG
        cout << "Send packet: " << string(fpk.packetBody, fpk.bodySize)
             << ". Try times [" << i+1 << "]" << endl;
    #endif
        this_thread::sleep_for(timeoutInterval);
        if (packetsConfirm.at(seqNumber)){
            delete packet; delete fpk.packetBody; return;
        }
    }
    delete packet; delete fpk.packetBody;
    throw SocketException("Lose connection. Seq: " + to_string(fpk.seqNumber), true);
}

void ReliableSocket::sendSinglePacket(ReliableSocket::formatPacket fpk, bool &successCheck)
throw(SocketException) {
    char *packet = deparsePacket(fpk);
    for (unsigned int i = 0; i < retryTimes; ++i) {
        send(packet, fpk.bodySize + 6);
    #ifdef RELIABLE_DEBUG
        cout << "Send packet: " << string(fpk.packetBody, fpk.bodySize)
            << ". Try times [" << i+1 << "]" << endl;
    #endif
        this_thread::sleep_for(timeoutInterval);
        if (successCheck) {delete packet; return;}
    }
    delete packet;
    throw SocketException("Lose connection. Flag: " + to_string(fpk.flag), true);
}