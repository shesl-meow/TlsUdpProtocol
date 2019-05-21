//
// Created by shesl-meow on 19-5-21.
//

#include "ReliableSocket.h"

#include <iostream>         // cerr
#include <string>
#include <cstring>          // memcpy()
#include <fstream>          // ifstream configFile
#include <streambuf>        // istreambuf_iterator
#include "json/json.h"      // Parse config string

// ReliableSocket Code

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
        }
        configFile.close();
    } else {
        cerr << "Can't open config file " << configPath << endl;
    }
    if (timeoutInterval == 0ms) timeoutInterval = 1s;   // Set default timeoutInterval to 1s
    if (packetSize == 0) packetSize = 1024;             // Set default packetSize to 1024 bytes = 1 kbytes
}

void ReliableSocket::setPackets(const string &messageBody) {
    auto messageLength = messageBody.size();
    for(int i = 0; i < messageLength / packetSize; ++i){
        char *packet = new char[packetSize];
        memcpy(packet, messageBody.substr(i * packetSize, packetSize).c_str(), packetSize);
        packetsBuffer.push_back(packet);
    }
    if(messageLength % packetSize != 0){
        char *packet = new char[packetSize];
        memcpy(packet, messageBody.substr(messageLength - messageLength % packetSize).c_str(), packetSize);
        packetsBuffer.push_back(packet);
    }
}

