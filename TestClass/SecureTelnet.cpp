//
// Created by shesl-meow on 19-5-29.
//

#include "../SecureSocket.h"
#include <iostream>

int main(int argc, char *argv[]){
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <Server Address> <Server Port>" << endl;
        exit(1);
    }

    string serverAddress = argv[1];
    unsigned short serverPort = atoi(argv[2]);     // First arg:  local port
    SecureSocket ss("/media/data/program/git/TlsUdpProtocol/config.json");
    ss.connectForeignAddressPort(serverAddress, serverPort);
    while (true) {
        string message;
        cout << "> "; cin >> message;
        ss.setPackets(message);
        ss.sendMessage();
    }
    return 0;
}