//
// Created by shesl-meow on 19-5-21.
//

#include "SecureSocket.h"

#include <gmp.h>
#include <iostream>
#include <sstream>
#include <time.h>
#include <assert.h>

#define PUB_FLAG 0x80u
#define SEC_FLAG 0x40u
#define SECURE_DEBUG

void SecureSocket::getPublicPacket(char *destBuffer, unsigned int destSize) const {
    for(unsigned int i = 0; i < destSize; ++i) destBuffer[i] = 0x00;
    assert(destSize >= 2*(primeBitsLength/8) + 4);
    *((unsigned short*)destBuffer) = 2*(primeBitsLength/8);
    *((unsigned short*)destBuffer + 1) = PUB_FLAG;

    size_t writenSize;
    mpz_export(destBuffer + 4, &writenSize, -1, sizeof(char), -1, 0, publicPrimeG);
    assert(writenSize <= primeBitsLength);
    mpz_export(destBuffer + (primeBitsLength/8) + 4, &writenSize, -1, sizeof(char), -1, 0, publicPrimeP);
    assert(writenSize <= primeBitsLength);
}

void SecureSocket::parsePublicPacket(const char *srcBuffer) throw(SocketException){
    if ( (*((unsigned short*)srcBuffer + 1) ^ PUB_FLAG) != 0u)
        throw SocketException("Try to parse a non-public packet.");
    unsigned short bodySize = *((unsigned short*)srcBuffer);
    if (bodySize != 2*(primeBitsLength/8)){
        stringstream ss;
        ss << "Server primeBitsLength=" << (bodySize*8)/2 << "; While client=" << primeBitsLength;
        throw SocketException(ss.str());
    }

    mpz_import(publicPrimeG, primeBitsLength, -1, sizeof(char), -1, 0, srcBuffer + 4);
    mpz_import(publicPrimeP, primeBitsLength, -1, sizeof(char), -1, 0, srcBuffer + (primeBitsLength/8) + 4);
#ifdef SECURE_DEBUG
    cout << "[Sync server prime] G:" << publicPrimeG << endl;
    cout << "[Sync server prime] P:" << publicPrimeP << endl;
#endif
}

void SecureSocket::getPrivatePacket(char *destBuffer, unsigned int destSize) const {
    for(unsigned int i = 0; i < destSize; ++i) destBuffer[i] = 0x00;
    assert(destSize >= (primeBitsLength/8) + 4);
    *((unsigned short*)destBuffer) = (primeBitsLength/8);
    *((unsigned short*)destBuffer + 1) = SEC_FLAG;

    mpz_t gxp; mpz_init(gxp);
    mpz_powm(gxp, publicPrimeG, privateXNumber, publicPrimeP);
    size_t writenSize;
    mpz_export(destBuffer+4, &writenSize, -1, sizeof(char), -1, 0, gxp);
    assert(writenSize <= primeBitsLength);
    mpz_clear(gxp);
}

void SecureSocket::parsePrivatePacket(const char *srcBuffer) throw(SocketException) {
    if ( (*((unsigned short*)srcBuffer + 1) ^ SEC_FLAG) != 0u)
        throw SocketException("Try to parse a non-private packet.");
    unsigned short bodySize = *((unsigned short*)srcBuffer);
    if (bodySize != primeBitsLength){
        stringstream ss;
        ss << "Server primeBitsLength=" << bodySize << "; While client=" << primeBitsLength;
        throw SocketException(ss.str());
    }

    mpz_t gyp; mpz_init(gyp);
    mpz_import(gyp, primeBitsLength, -1, sizeof(char), -1, 0, srcBuffer+4);
    mpz_powm(exchangedKey, gyp, privateXNumber, publicPrimeP);
    mpz_clear(gyp);
#ifdef SECURE_DEBUG
    cout << "[Grab key] DH algorithm:" << exchangedKey << endl;
#endif
}

SecureSocket::SecureSocket(const char *configPath)
throw(SocketException): ReliableSocket(){
    loadConfig(configPath);
}

SecureSocket::SecureSocket(unsigned short localPort, const char *configPath)
throw(SocketException): ReliableSocket(localPort) {
    loadConfig(configPath);
}

SecureSocket::SecureSocket(const string &localAddress, unsigned short localPort, const char *configPath)
throw(SocketException): ReliableSocket(localAddress, localPort) {
    loadConfig(configPath);
}

SecureSocket::~SecureSocket() {
    mpz_clear(publicPrimeP);
    mpz_clear(publicPrimeG);
    mpz_clear(privateXNumber);
    mpz_clear(exchangedKey);
}

Json::Value SecureSocket::loadConfig(const char *configPath) throw(SocketException) {
    mpz_inits(publicPrimeP, publicPrimeG, privateXNumber, exchangedKey, NULL);
    // TODO: initialize random requirement
    gmp_randstate_t r_state;
    gmp_randinit_default(r_state);
    gmp_randseed_ui(r_state, time(0));

    auto configVal = ReliableSocket::loadConfig(configPath);
    primeBitsLength = configVal.get("primeBitsLength", 1024).asInt();
    if (primeBitsLength % 8 != 0)
        throw SocketException("Please provide a multiple of 8 bits length.");

    mpz_set_str(publicPrimeG, configVal.get("publicPrimeG", "65537").asCString(), 10);
    if(mpz_probab_prime_p(publicPrimeG, 10) == 0){
        stringstream ss;
        ss << "Public prime g " << publicPrimeG << " isn't prime";
        throw SocketException(ss.str());
    }

    mpz_set_str(publicPrimeP, configVal.get("publicPrimeP", "0").asCString(), 10);
    if (mpz_cmp_ui(publicPrimeP, 0) == 0) {
        // TODO: generate 1024 bits random number as publicPrimeP
        mpz_urandomb(publicPrimeP, r_state, primeBitsLength);
        mpz_nextprime(publicPrimeP, publicPrimeP);
    } else if (mpz_probab_prime_p(publicPrimeP, 10) == 0){
        stringstream ss;
        ss << "Public prime p " << publicPrimeP << endl;
        throw SocketException(ss.str());
    }

    mpz_urandomm(privateXNumber, r_state, publicPrimeP);
#ifdef SECURE_DEBUG
    cout << "[Set public prime] G:" << publicPrimeG << endl;
    cout << "[Set public prime] P:" << publicPrimeP << endl;
    cout << "[Grab private] X:" << privateXNumber << endl;
#endif
    gmp_randclear(r_state);
    return configVal;
}

void SecureSocket::startListen() throw(SocketException){
    ReliableSocket::startListen();
    // TODO: STEP1 -- Send public message to client side.
    char* pubpacket = new char [(primeBitsLength/8)*2 + 4];
    getPublicPacket(pubpacket, (primeBitsLength/8)*2 + 4);
    this->setPackets(pubpacket, (primeBitsLength/8)*2 + 4);
    ReliableSocket::sendMessage();
    delete []pubpacket;
    // TODO: STEP2 -- Receive private message from client side.
    ReliableSocket::receiveMessage();
    char *prvmessage = new char [messageLength];
    this->readMessage(prvmessage, messageLength);
    parsePrivatePacket(prvmessage);
    delete []prvmessage;
    // TODO: STEP3 -- Send private back
    char* prvpacket = new char [(primeBitsLength/8) + 4];
    getPrivatePacket(prvpacket, (primeBitsLength/8) + 4);
    this->setPackets(prvpacket, (primeBitsLength/8) + 4);
    ReliableSocket::sendMessage();
    delete []prvpacket;
}

void SecureSocket::connectForeignAddressPort(const string &address, unsigned short port)
throw(SocketException) {
    ReliableSocket::connectForeignAddressPort(address, port);
    // TODO: STEP1 -- Receive public message from peer side.
    ReliableSocket::receiveMessage();
    char *pubmessage = new char [messageLength];
    this->readMessage(pubmessage, messageLength);
    parsePublicPacket(pubmessage);
    delete []pubmessage;
    // TODO: STEP2 -- Send private message of client side.
    char *prvpacket = new char [(primeBitsLength/8) + 4];
    getPrivatePacket(prvpacket, (primeBitsLength/8) + 4);
    this->setPackets(prvpacket, (primeBitsLength/8) + 4);
    ReliableSocket::sendMessage();
    delete []prvpacket;
    // TODO: STEP3 -- Receive private packet from server side.
    ReliableSocket::receiveMessage();
    char *prvmessage = new char [messageLength];
    this->readMessage(prvmessage, messageLength);
    parsePrivatePacket(prvmessage);
    delete []prvmessage;
}