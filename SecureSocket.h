//
// Created by shesl-meow on 19-5-21.
//

#ifndef TLSUDPPROTOCOL_SECURESOCKET_H
#define TLSUDPPROTOCOL_SECURESOCKET_H

#include "./ReliableSocket.h"

#include <gmp.h>    // for big integer

/**
 * A public parameter g used in DH-KEY-EXCHANGE
 */
#define PUBLIC_PRIME_G 65537

class SecureSocket : public ReliableSocket{
private:
    /**
     * Get public packet which is used in agree on public g and p
     * @param destBuffer destination buffer
     * @param destSize  destination buffer size
     */
    void getPublicPacket (char* destBuffer, unsigned int destSize) const;

    /**
     * Parse the public packet from the peer side, client grab public p&g from server
     * @param srcBuffer source buffer
     * @param srcSize source buffer size
     */
    void parsePublicPacket (const char* srcBuffer) throw(SocketException);

    /**
     * Get private packet which is used to send (g^X mod p)
     * @param destBuffer destination buffer
     * @param destSize destination buffer size
     */
    void getPrivatePacket (char* destBuffer, unsigned int destSize) const;

    /**
     * Parse the private packet from the peer side, client grab private p&g from server
     * @param srcBuffer source buffer
     * @param srcSize source buffer size
     */
    void parsePrivatePacket (const char* srcBuffer) throw(SocketException);

public:
    /**
     *   Construct a secure socket
     *   @exception SocketException thrown if unable to create secure socket
     */
    SecureSocket(const char *configPath) throw(SocketException);

    /**
     *   Construct a secure socket with the given local port
     *   @param localPort local ports
     *   @exception SocketException thrown if unable to create reliable UDP socket
     */
    SecureSocket(unsigned short localPort, const char *configPath = "./config.json")
    throw(SocketException);

    /**
     *   Construct a secure socket with the given local port and address
     *   @param localAddress local address
     *   @param localPort local port
     *   @exception SocketException thrown if unable to create secure socket
     */
    SecureSocket(const string &localAddress, unsigned short localPort,
                   const char *configPath = "./config.json") throw(SocketException);

    /**
     * release memory allocated for gmp big integer.
     */
    ~SecureSocket();

    /**
     * Load config from a json file. Load reliable config and then load secure config:
     *   publicPrimeP, publicPrimeG: if not prime - raise error, if not provided p=random(1024) g=65537
     * @param configPath configuration file path.
     */
     Json::Value loadConfig(const char *configPath) throw(SocketException) override;

     void startListen() throw(SocketException) override;

    void connectForeignAddressPort (const string& address, unsigned short port) throw(SocketException) override ;

protected:
    /**
     * primeBitsLength: the max bit length of the prime number in crypto
     */
    unsigned int primeBitsLength = 1024;

    /**
     * Public prime g,p implement in DH algorithm, both of them are primes.
     * They can be loaded from config.json file. By default g=65537, p=getPrime(1024)
     */
    mpz_t publicPrimeG = {0};
    mpz_t publicPrimeP = {0};

    /**
     * Secret random number implement in DH algorithm.
     * This must be generated when SecureSocket in init, secret=getNumber(1024)
     */
    mpz_t privateXNumber;

    /**
     * The exchange result in DH algorithm,
     * Cut the last 256 bits if result is far more than that.
     */
    mpz_t exchangedKey;
};


#endif //TLSUDPPROTOCOL_SECURESOCKET_H
