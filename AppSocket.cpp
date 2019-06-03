//
// Created by lanshuhan on 19-6-2.
//

#include "AppSocket.h"
#include <iostream>
#include <fstream>
#include <memory.h>
#include <openssl/sha.h>
#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif

// HEADER signature bits
#define JOIN_REQ 0x01u
#define PASS_REQ 0x02u
#define PASS_RESP 0x04u
#define PASS_ACCEPT 0x08u
#define DATA 0x010u
#define TERMINATE 0x20u
#define PASS_REJECT 0x40u

#define APP_DEBUG true

AppSocket::formatPacket AppSocket::parsePacket(const char *pac) {
    formatPacket fpac;
    fpac.header = *(unsigned short *)pac;
    fpac.payloadLength = *(unsigned int*)(pac + 2);

    if (fpac.header == PASS_RESP || fpac.header == TERMINATE) {
        fpac.body = new char[fpac.payloadLength];
        memcpy(fpac.body, pac + 6, fpac.payloadLength);
    }
    else if (fpac.header == DATA)
    {
        fpac.body = new char[fpac.payloadLength];
        memcpy(fpac.body, pac + 6, fpac.payloadLength);
    }
    else
        fpac.body = nullptr;
    return fpac;
}

char* AppSocket::deparsePacket(const AppSocket::formatPacket fpac)
{
    char* pac = new char [6 + fpac.payloadLength];
    if (fpac.header == DATA)
    {
        *((unsigned short *)pac) = fpac.header;
        *(unsigned int *)((unsigned short *)pac + 1) = fpac.payloadLength;
        memcpy(pac + 6, fpac.body, fpac.payloadLength);
    }
    else if (fpac.header == PASS_RESP || fpac.header == TERMINATE)
    {
        *((unsigned short *)pac) = fpac.header;
        *(unsigned int *)((unsigned short *)pac + 1) = fpac.payloadLength;
        memcpy(pac + 6, fpac.body, fpac.payloadLength);
    }
    else
    {
        *((unsigned short *)pac) = fpac.header;
        *(unsigned int *)((unsigned short *)pac + 1) = fpac.payloadLength;
    }
    return pac;
}

AppSocket::formatPacket AppSocket::getShortPacket(unsigned short type)
{
    formatPacket fpac;
    fpac.header = type;
    fpac.payloadLength = 0;
    return fpac;
}
AppSocket::formatPacket AppSocket::getMediumPacket(
        unsigned short type, unsigned int length, char* content)
{
    //length(bytes)
    formatPacket fpac;
    fpac.header = type;
    fpac.payloadLength = length;
    fpac.body = new char[length];
    memcpy(fpac.body, content, length);
    return fpac;
}
AppSocket::formatPacket AppSocket::getLongPacket(unsigned short type, unsigned int length, unsigned int pID, char* content)
{
    formatPacket fpac;
    fpac.header = type;
    fpac.payloadLength = length;
    fpac.body = new char[length];
    memcpy(fpac.body, content, length);
    return fpac;
}

char* AppSocket::getCharPacket(unsigned short type)
{
    formatPacket fpac = getShortPacket(type);
    char* pac = deparsePacket(fpac);
    return pac;
}

char* AppSocket::getCharPacket(unsigned short type, unsigned int length, char* content)
{
    formatPacket fpac = getMediumPacket(type, length, content);
    char* pac = deparsePacket(fpac);
    return pac;
}

char* AppSocket::getCharPacket(unsigned short type, unsigned int length, unsigned int pID, char* content)
{
    formatPacket fpac = getLongPacket(type, length, pID, content);
    char* pac = deparsePacket(fpac);
    return pac;
}

void AppSocket::getHashValue(const char *srcBuffer,
        size_t srcSize, unsigned char destBuffer[20]) {
    SHA_CTX	c; SHA1_Init(&c);
    char *saltSrc = new char [srcSize + hashSalt.size()];
    memcpy(saltSrc, srcBuffer, srcSize);
    memcpy(saltSrc + srcSize, hashSalt.c_str(), hashSalt.size());
    SHA1_Update(&c, srcBuffer, srcSize);
    SHA1_Final(destBuffer, &c);
    delete []saltSrc;
}

bool AppSocket::checkPassword(const unsigned char *passHashBuffer, size_t bufferSize) {
    unsigned char correctHashBuffer[20];
    getHashValue(bindPassword, strlen(bindPassword), correctHashBuffer);
    return (strcmp(reinterpret_cast<const char*>(passHashBuffer),
                       reinterpret_cast<const char*>(correctHashBuffer)) == 0);
}

AppSocket::AppSocket(const char *configPath) : SecureSocket(configPath) {}

AppSocket::AppSocket(unsigned short localPort, const char *configPath) : SecureSocket(localPort, configPath) {}

AppSocket::AppSocket(const string &localAddress, unsigned short localPort,
        const char *configPath) : SecureSocket(localAddress, localPort, configPath) {}

AppSocket::~AppSocket() {
    delete []fileContent;
    delete []bindPassword;
}

void AppSocket::loadFile(const char *filename) {
    ifstream infile(filename);
    if (!infile.is_open())
        throw SocketException("Counldn't open file " + string(filename), true);

    // TODO: get file size in bytes
    infile.seekg(0, infile.end);
    auto length = infile.tellg();
    infile.seekg(0, infile.beg);

    this->fileContent = new char [length];
    infile.read(this->fileContent, length);
    contentLength = length;
    infile.close();
#ifdef APP_DEBUG
    cout << "[Load File] load " << length << " bytes from " << filename << endl;
#endif
}

void AppSocket::writeFile(const char *filename) const {
    ofstream outfile(filename);
    if (!outfile.is_open())
        throw SocketException("Counldn't open file " + string(filename), true);

    outfile.write(fileContent, contentLength);
    outfile.close();
#ifdef APP_DEBUG
    cout << "[Write File] write " << contentLength << " bytes to " << filename << endl;
#endif
}

void AppSocket::setPassword(const char *password, size_t passwdLen) {
    delete []bindPassword;
    bindPassword = new char [passwdLen];
    memcpy(bindPassword, password, passwdLen);
}

void AppSocket::startListen() {
    if (bindPassword == nullptr)
        throw SocketException("Please bind password before listen.", false);
    SecureSocket::startListen();
    // TODO: STEP1 -- received JOIN_REQ packet from client side.
    SecureSocket::receiveMessage();
    char *buffer = new char [messageLength + 1];
    this->readMessage(buffer, messageLength + 1);
    auto fpacket = parsePacket(buffer);
    if ((fpacket.header & JOIN_REQ) == 0u)
        throw SocketException("Received a none JOIN_REQ packet.");
#ifdef APP_DEBUG
    cout << "[Receive message] JOIN_REQ packet received: " << endl;
#endif
    delete []buffer; delete []fpacket.body;

    // TODO: STEP2 -- send PASS_REQ packet to client side
    auto passPacket = getShortPacket(PASS_REQ);
    auto ppacket = deparsePacket(passPacket);
    this->setPackets(ppacket, passPacket.payloadLength + 6);
    SecureSocket::sendMessage();
    delete []passPacket.body; delete []ppacket;
}

void AppSocket::startAuthenticate() {
    // TODO: STEP1 -- receive & validate for `passMaxTryTimes`.
    auto rejectPacket = getShortPacket(PASS_REJECT);
    auto acceptPacket = getShortPacket(PASS_ACCEPT);
    char *rpacket = deparsePacket(rejectPacket);
    char *apacket = deparsePacket(acceptPacket);
    for(passwdTryTimes = 0; passwdTryTimes < passwdMaxTryTimes; ++passwdTryTimes){
        SecureSocket::receiveMessage();
        auto buffer = new char [messageLength + 1];
        auto prpacket = parsePacket(buffer);
        if ((prpacket.header & PASS_RESP) == 0u)
            throw SocketException("Received a none PASS_RESP packet.");
        delete []buffer;
        if (checkPassword(
                reinterpret_cast<const unsigned char *>(prpacket.body),
                prpacket.payloadLength)
                ) {
        #ifdef APP_DEBUG
            cout << "[Password received] Client password has been accept. [Times "
                 << passwdTryTimes << "/" << passwdMaxTryTimes << "]" << endl;
        #endif
            delete []prpacket.body;
            this->setPackets(apacket, acceptPacket.payloadLength + 6);
            SecureSocket::sendMessage();
            break;
        } else {
        #ifdef APP_DEBUG
            cout << "[Password received] Client send a invalid password. [Times "
                 << passwdTryTimes << "/" << passwdMaxTryTimes << "]" << endl;
        #endif
            delete []prpacket.body;
            this->setPackets(rpacket, rejectPacket.payloadLength + 6);
            SecureSocket::sendMessage();
        }
    }
    delete []rejectPacket.body; delete []acceptPacket.body;
    delete []rpacket; delete []apacket;

    // TODO: STEP2 -- terminate if max try times reach
    if (passwdTryTimes == passwdMaxTryTimes) {
    #ifdef APP_DEBUG
        cout << "[Password received] Reach the max try times. Terminate." << endl;
    #endif
        auto terminatePacket = getMediumPacket(TERMINATE, 0, nullptr);
        auto tpacket = deparsePacket(terminatePacket);
        this->setPackets(tpacket, terminatePacket.payloadLength + 6);
        SecureSocket::sendMessage();
        delete []tpacket; delete []terminatePacket.body;
    }
}

void AppSocket::connectForeignAddressPort(const string &address, unsigned short port) {
    SecureSocket::connectForeignAddressPort(address, port);

    // TODO: STEP1 -- send JOIN_REQ packet to server side.
    auto joinPacket = getShortPacket(JOIN_REQ);
    auto jpacket = deparsePacket(joinPacket);
    this->setPackets(jpacket, joinPacket.payloadLength + 6);
    SecureSocket::sendMessage();
    delete []joinPacket.body; delete []jpacket;
#ifdef APP_DEBUG
    cout << "[Send Message] sent JOIN_REQ packet" << endl;
#endif

    // TODO: STEP2 -- received PASS_REQ packet from server side.
    char *buffer
}

int AppSocket::connectToServer(const char* passCat, int passLen, const char* add)
{
    char* sendContent;
    char* getContent = new char[DATA_SIZE];
    char* readbuf = new char[READ_BUF_SIZE];
    //char* pac;
    //formatPacket getPac;
    unsigned short type;
    unsigned int length;
    unsigned int pID;

    //send JOIN_REQ
    sendContent = getCharPacket(JOIN_REQ);
    setPackets(sendContent, 6);
    sendMessage();

    //recv PASS_REQ, send PASS_RESP
    receiveMessage();
    readMessage(readbuf, READ_BUF_SIZE);
    recvPacket(readbuf, type, length, pID, getContent);
    if (type != PASS_REQ)
    {
        cout << "ABORT" << endl;
        return -1;
    }
    else
    {
        char* temp = new char[passLen];
        strcpy(temp, passCat);
        sendContent = getCharPacket(PASS_RESP, passLen, temp);
        setPackets(sendContent, 6 + passLen);
        sendMessage();
    }

    //recv PASS_ACCEPT, wait for DATA & TERMINATE
    receiveMessage();
    readMessage(readbuf, READ_BUF_SIZE);
    recvPacket(readbuf, type, length, pID, getContent);
    if (type != PASS_ACCEPT)
    {
        if (type == PASS_REJECT)
            cout << "pw error" << endl;
        cout << "ABORT" << endl;
        return -1;
    }
    else
    {
        char* wbuf = new char[STRING_MAX];
        wbuf[0] = 0;
        while (true)
        {
            int tempID = 1;
            receiveMessage();
            readMessage(readbuf, READ_BUF_SIZE);
            recvPacket(readbuf, type, length, pID, getContent);
            if (pID != 0 && tempID != pID)
            {
                cout << "ABORT" << endl;
                return -1;
            }
            tempID++;
            if (type == TERMINATE)
            {
                char* sha = sha1(wbuf);
                for (int i = 0; i < 20; i++)
                {
                    if (sha[i] != getContent[i])
                    {
                        cout << "ABORT" << endl;
                        return -1;
                    }
                }
                break;
            }
            else
            {
                strcat(wbuf, getContent);
            }
        }
        ofstream outfile;
        outfile.open(add);
        outfile << wbuf;
        outfile.close();
        cout << "OK" << endl;
    }
    return 0;
}

int AppSocket::connectToClient(const char* pass, int passLen, const char* add)
{
    char* sendContent;
    char* getContent = new char[DATA_SIZE];
    char* sendbuf = new char[DATA_SIZE];
    char* readbuf = new char[READ_BUF_SIZE];
    unsigned short type;
    unsigned int length;
    unsigned int pID;

    //recv JOIN_REQ, send PASS_REQ
    receiveMessage();
    readMessage(readbuf, READ_BUF_SIZE);
    recvPacket(readbuf, type, length, pID, getContent);
    if (type != JOIN_REQ)
    {
        cout << "ABORT" << endl;
        return -1;
    }
    else
    {
        sendContent = getCharPacket(PASS_REQ);
        setPackets(sendContent,6);
        sendMessage();
    }

    //recv PASS_RESP, send PASS_ACCEPT
    receiveMessage();
    readMessage(readbuf, READ_BUF_SIZE);
    recvPacket(readbuf, type, length, pID, getContent);
    if (type != PASS_RESP || !checkPassword(getContent, length, pass, passLen))
    {
        sendContent = getCharPacket(PASS_REJECT);
        setPackets(sendContent);
        sendMessage();
        cout << "ABORT(wrong pw)" << endl;
        return -1;
    }
    else
    {
        sendContent= getCharPacket(PASS_ACCEPT);
        setPackets(sendContent,6);
        sendMessage();

        ifstream infile;
        infile.open(add);
        int data_len;
        for (int i = 1;; i++)
        {
            infile.get(sendbuf, DATA_SIZE, EOF);
            data_len = strlen(sendbuf);
            if (data_len == 0)
                break;
            sendContent = getCharPacket(DATA, data_len, i, sendbuf);
            setPackets(sendContent,6+data_len);
            sendMessage();
            if (data_len < DATA_SIZE)
                break;
        }
        infile.seekg(0,ios::beg);
        char entire[STRING_MAX];
        infile.get(entire, STRING_MAX, EOF);
        char* shaResult = sha1(entire);
        sendContent = getCharPacket(TERMINATE, 20, shaResult);
        setPackets(sendContent,6+20);
        sendMessage();
        cout << "OK" << endl;
        return 0;
    }
}


void AppSocket::recvPacket(char* target, unsigned short& type, unsigned int& length, unsigned int& pID, char* content)
{
    formatPacket fpac = parsePacket(target);
    type = fpac.header;
    length = fpac.payloadLength;
    if (fpac.header == DATA)
    {
        memset(content, 0, DATA_SIZE);
        memcpy(content, fpac.body, fpac.payloadLength);
    }
    else if (fpac.header == TERMINATE || fpac.header == PASS_RESP)
    {
        pID = 0;
        memcpy(content, fpac.body, fpac.payloadLength);
    }
    else
    {
        pID = 0;
    }
}

bool AppSocket::checkPassword(const char* pw3, int len3, const char* pw, int len)
{
    //cout << pw3 << endl << pw << endl;
    for (int i = 0; i < len3; i++)
    {
        for (int j = 0; j < len; j++)
        {
            if (pw3[j+i] != pw[j])
                break;
            if (j == len - 1)
                return true;
        }
        int j;
        for (j = i; j < len3 && pw3[j] != ','; j++)
        {}
        i = j;
    }
    return false;
}

char* AppSocket::sha1(char* src)
{
    char* wbuff=new char[20];
    SHA_CTX	c;
    memset(wbuff, 0, sizeof(wbuff));
    SHA1_Init(&c);
    SHA1_Update(&c, src, strlen(src));
    SHA1_Final((unsigned char*)wbuff, &c);
    return wbuff;
}