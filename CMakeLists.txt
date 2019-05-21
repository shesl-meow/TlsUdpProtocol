cmake_minimum_required(VERSION 3.14)
project(TlsUdpProtocol)

set(CMAKE_CXX_STANDARD 14)

add_executable(udptelnet TestClass/UdpTelnet.cpp UdpSocket.cpp)
add_executable(udpserver TestClass/UdpServer.cpp UdpSocket.cpp)

add_executable(reliableserver TestClass/ReliableServer.cpp ReliableSocket.cpp UdpSocket.cpp)
target_link_libraries(reliableserver jsoncpp)