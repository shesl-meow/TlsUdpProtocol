cmake_minimum_required(VERSION 3.14)
project(TlsUdpProtocol)

set(CMAKE_CXX_STANDARD 14)

add_executable(udptelnet TestClass/UdpTelnet.cpp UdpSocket.cpp)
add_executable(udpserver TestClass/UdpServer.cpp UdpSocket.cpp)

add_executable(reliableserver TestClass/ReliableServer.cpp ReliableSocket.cpp UdpSocket.cpp)
target_link_libraries(reliableserver jsoncpp pthread)

add_executable(reliabletelnet TestClass/ReliableTelnet.cpp ReliableSocket.cpp UdpSocket.cpp)
target_link_libraries(reliabletelnet jsoncpp pthread)

add_executable(secureserver TestClass/SecureServer.cpp SecureSocket.cpp ReliableSocket.cpp UdpSocket.cpp)
target_link_libraries(secureserver jsoncpp pthread gmp gmpxx crypto)

add_executable(securetelnet TestClass/SecureTelnet.cpp SecureSocket.cpp ReliableSocket.cpp UdpSocket.cpp)
target_link_libraries(securetelnet jsoncpp pthread gmp gmpxx crypto)