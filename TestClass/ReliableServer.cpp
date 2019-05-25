//
// Created by shesl-meow on 19-5-21.
//
#include "../ReliableSocket.h"
#include <iostream>             // For cout and cerr
#include <cstdlib>              // For atoi()

int main(int argc, char *argv[]){
    if (argc != 3) {                  // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <Server Address> <Server Port>" << endl;
        exit(1);
    }

    string serverAddress = argv[1];
    unsigned short serverPort = atoi(argv[2]);     // First arg:  local port
    try {
        ReliableSocket sock("/media/data/program/git/TlsUdpProtocol/config.json");
        sock.bindLocalAddressPort(serverAddress, serverPort);
        sock.startListen();

        while (true) {
            sock.receiveMessage();

            auto len = sock.getMessageLength();
            char *message = new char(len + 1);
            sock.readMessage(message, len + 1);
            cout << "received from " << sock.getPeerAddress() << ":" <<
                sock.getPeerPort() << ": " << message << endl;
            delete []message;
        }
    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }
    return 0;
}
