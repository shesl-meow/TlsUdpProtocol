//
// Created by shesl-meow on 19-5-28.
//
#include "../include/SecureSocket.h"
#include <iostream>

int main(int argc, char *argv[]){
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <Server Address> <Server Port>" << endl;
        exit(1);
    }

    string serverAddress = argv[1];
    unsigned short serverPort = atoi(argv[2]);     // First arg:  local port
    SecureSocket ss("/media/data/program/git/TlsUdpProtocol/config.json");
    ss.bindLocalAddressPort(serverAddress, serverPort);
    ss.startListen();

    while (true) {
        ss.receiveMessage();
        char *message = new char [ss.getMessageLength() + 1];
        ss.readMessage(message, ss.getMessageLength() + 1);
        cout << "Receiving from " << ss.getForeignAddress() << ":" << ss.getForeignPort()
            << ": " << string(message, ss.getMessageLength()) << endl;
        delete []message;
    }
    return 0;
}