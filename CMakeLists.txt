cmake_minimum_required(VERSION 3.14)
project(TlsUdpProtocol)

set(CMAKE_CXX_STANDARD 14)

add_executable(udptelnet TestClass/UdpTelnet.cpp src/UdpSocket.cpp)
add_executable(udpserver TestClass/UdpServer.cpp src/UdpSocket.cpp)

add_executable(reliableserver TestClass/ReliableServer.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(reliableserver jsoncpp pthread)
add_executable(reliabletelnet TestClass/ReliableTelnet.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(reliabletelnet jsoncpp pthread)

add_executable(secureserver TestClass/SecureServer.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(secureserver jsoncpp pthread gmp gmpxx crypto)
add_executable(securetelnet TestClass/SecureTelnet.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(securetelnet jsoncpp pthread gmp gmpxx crypto)

add_executable(appserver Server/AppServer.cpp src/AppSocket.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(appserver jsoncpp pthread gmp gmpxx crypto)
add_executable(appclient Client/AppClient.cpp src/AppSocket.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(appclient jsoncpp pthread gmp gmpxx crypto)
