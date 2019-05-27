//
// Created by shesl-meow on 19-5-22.
//

#include "../ReliableSocket.h"      // For UDPSocket and SocketException
#include <iostream>               // For cout and cerr
#include <cstring>                // For strlen()

using namespace std;

const int ECHOMAX = 255;          // Longest string to echo

int main(int argc, char *argv[]) {
    if (argc != 3) {              // Test for correct number of arguments
        cerr << "Usage: " << argv[0]
             << " <Server> <Server Port>\n";
        exit(1);
    }

    string servAddress = argv[1], echoString;
    unsigned short servPort = Socket::resolveService(argv[2], "udp");

    try {
        ReliableSocket sock("/media/data/program/git/TlsUdpProtocol/config.json");
        // sock.connect(servAddress, servPort);
        sock.connectForeignAddressPort(servAddress, servPort);
        while(true){
            cout << "> "; cin >> echoString;

            // Send the string to the Server
            sock.setPackets(echoString);
            sock.sendMessage();
        }
    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}