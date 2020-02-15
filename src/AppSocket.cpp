//
// Created by lanshuhan on 19-6-2.
//

#include "../include/AppSocket.h"
#include <iostream>
#include <fstream>
#include <memory.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif

// HEADER signature bits
#define JOIN_REQ 0x01
#define PASS_REQ 0x02
#define PASS_RESP 0x04
#define PASS_ACCEPT 0x08
#define DATA 0x010
#define TERMINATE 0x20
#define REJECT 0x40
#define DATA_SUC 0x80

AppSocket::formatPacket AppSocket::parsePacket(const char *pac) {
    formatPacket fpac;
    fpac.header = *(unsigned short *)pac;
    fpac.payloadLength = *(unsigned int*)((unsigned short *)pac + 1);
    if (fpac.header == DATA)
        fpac.packetID = *(unsigned int*)((unsigned short *)pac + 3);

    if (fpac.header == PASS_RESP || fpac.header == TERMINATE) {
        fpac.body = new char[fpac.payloadLength];
        memcpy(fpac.body, pac + 6, fpac.payloadLength);
    }
    else if (fpac.header == DATA)
    {
        fpac.body = new char[fpac.payloadLength];
        memcpy(fpac.body, pac + 10, fpac.payloadLength);
    }
    else
        fpac.body = nullptr;
    return fpac;
}

char* AppSocket::deparsePacket(const AppSocket::formatPacket fpac)
{
    char* pac;
    if (fpac.header == DATA)
    {
        pac = new char[10 + fpac.payloadLength];
        *((unsigned short *)pac) = fpac.header;
        *(unsigned int *)((unsigned short *)pac + 1) = fpac.payloadLength;
        *(unsigned int *)((unsigned short *)pac + 3) = fpac.packetID;
        memcpy(pac + 10, fpac.body, fpac.payloadLength);
    }
    else if (fpac.header == PASS_RESP || fpac.header == TERMINATE)
    {
        pac = new char[6 + fpac.payloadLength];
        *((unsigned short *)pac) = fpac.header;
        *(unsigned int *)((unsigned short *)pac + 1) = fpac.payloadLength;
        memcpy(pac + 6, fpac.body, fpac.payloadLength);
    }
    else
    {
        pac = new char[6];
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
AppSocket::formatPacket AppSocket::getMediumPacket(unsigned short type, unsigned int length, char* content)
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
    fpac.packetID = pID;
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

AppSocket::AppSocket(const char *configPath) : SecureSocket(configPath) {}

AppSocket::AppSocket(unsigned short localPort, const char *configPath) : SecureSocket(localPort, configPath) {}

AppSocket::AppSocket(const string &localAddress, unsigned short localPort, const char *configPath) : SecureSocket(localAddress, localPort, configPath) {}

AppSocket::~AppSocket() {}

int AppSocket::connectToServer(const char* passCat, int passLen, const char* add)
{
    char* sendContent;
    char* getContent = new char[DATA_SIZE];
    char* readbuf = new char[READ_BUF_SIZE+1];
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
        if (type == REJECT)
            cout << "pw error" << endl;
        cout << "ABORT" << endl;
        return -1;
    }
    else
    {
        char* wbuf = new char[STRING_MAX];
		memset(wbuf, 0, STRING_MAX);
		int tempID = 1;
		bool isError = false;
        while (true)
        {
            
            receiveMessage();
			delete[]readbuf;
			readbuf = new char[READ_BUF_SIZE + 1];
            readMessage(readbuf, READ_BUF_SIZE+1);
            recvPacket(readbuf, type, length, pID, getContent);
            if (pID != 0 && tempID != pID)
            {
				isError = true;
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
						isError = true;
                    }
                }
                break;
            }
            else
            {
				memcpy(wbuf + (tempID - 2) * 1000, getContent, 1000);
			}
        }
        ofstream outfile;
        outfile.open(add);
        outfile << wbuf;
        outfile.close();
		
		if (isError == false)
		{
			sendContent = getCharPacket(DATA_SUC);
			setPackets(sendContent, 6);
			sendMessage();
			cout << "OK" << endl;
		}
		else
		{
			sendContent = getCharPacket(REJECT);
			setPackets(sendContent, 6);
			sendMessage();
			cout << "ABORT" << endl;
		}
    }
    return 0;
}

int AppSocket::connectToClient(const char* pass, int passLen, const char* add)
{
    char* sendContent;
    char* getContent = new char[DATA_SIZE];
    char* sendbuf = new char[DATA_SIZE+1];
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
        sendContent = getCharPacket(REJECT);
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
            infile.get(sendbuf, DATA_SIZE+1, EOF);
            data_len = strlen(sendbuf);
            if (data_len == 0)
                break;
            sendContent = getCharPacket(DATA, data_len, i, sendbuf);
            setPackets(sendContent,10+data_len);
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
		receiveMessage();
		readMessage(readbuf, READ_BUF_SIZE);
		recvPacket(readbuf, type, length, pID, getContent);
		if (type == DATA_SUC)
			cout << "OK" << endl;
		else
			cout << "ABORT" << endl;
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
        pID = fpac.packetID;
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