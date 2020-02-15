//
// Created by lanshuhan on 19-6-2.
//

#include "../include/AppSocket.h"
#include <iostream>         // cerr
#include <cstring>

int main(int argc, char* argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <Server Port> <Password> <input file>" << endl;
        exit(1);
    }
    int serverPort = atoi(argv[1]);
    int pwLen = strlen(argv[2]);
    AppSocket s("127.0.0.1", serverPort, "/media/data/program/git/TlsUdpProtocol/config.json");
    s.startListen();
    s.connectToClient(argv[2], pwLen, argv[3]);
}
