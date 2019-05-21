//
// Created by shesl-meow on 19-5-21.
//
#include "../ReliableSocket.h"
#include <iostream>             // For cout and cerr
#include <cstdlib>              // For atoi()

int main(int argc, char *argv[]){
    if (argc != 2) {                  // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
        exit(1);
    }

    unsigned short serverPort = atoi(argv[1]);     // First arg:  local port
    try {
        ReliableSocket sock(serverPort, "/media/data/program/git/TlsUdpProtocol/config.json");
        sock.setPackets("I am a pigger.");
    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }
    return 0;
}
