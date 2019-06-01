//
// Created by lanshuhan on 19-6-2.
//

#include "../AppSocket.h"

#include <cstring>      // strlen
#include <iostream>     // cerr

int main(int argc, char* argv[])
{
    if (argc < 4 || argc > 7) {
        cerr << "Usage: " << argv[0] << " <Server Address> <Server Port> "
            << "[<Passwd1> <Passwd2> <Passwd3>] <Output File>" << endl;
    }

    string serverAddress = argv[1];
    int serverPort = atoi(argv[2]);
    char* pwCat = new char[50];
    pwCat[0] = 0;
    strcat(pwCat, argv[3]);
    strcat(pwCat, ",");
    strcat(pwCat, argv[4]);
    strcat(pwCat, ",");
    strcat(pwCat, argv[5]);
    int len = strlen(argv[3]) + strlen(argv[4]) + strlen(argv[5]) + 2;
    AppSocket c("/media/data/program/git/TlsUdpProtocol/config.json");
    c.connectForeignAddressPort(argv[1], serverPort);
    c.connectToServer(pwCat, len, argv[6]);
}