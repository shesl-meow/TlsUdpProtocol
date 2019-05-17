cmake_minimum_required(VERSION 3.14)
project(tlsUdpProtocol)

set(CMAKE_CXX_STANDARD 14)

add_executable(udptelnet udpTelnet/udpTelnet.cpp UDPSocket.cpp)