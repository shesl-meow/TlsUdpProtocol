//
// Created by shesl-meow on 19-5-21.
//

#ifndef TLSUDPPROTOCOL_SECURESOCKET_H
#define TLSUDPPROTOCOL_SECURESOCKET_H

#include "./ReliableSocket.h"

/**
 * A public parameter g used in DH-KEY-EXCHANGE
 */
#define PUBLIC_PRIME_G 65537

class SecureSocket : public ReliableSocket{
private:

public:
    /**
     *   Construct a secure socket
     *   @exception SocketException thrown if unable to create secure socket
     */
    SecureSocket(const char *configPath = "./config.json") throw(SocketException);
};


#endif //TLSUDPPROTOCOL_SECURESOCKET_H
