cmake_minimum_required(VERSION 3.14)
project(TlsUdpProtocol)

set(CMAKE_CXX_STANDARD 14)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(udptelnet app/UdpTelnet.cpp src/UdpSocket.cpp)
add_executable(udpserver app/UdpServer.cpp src/UdpSocket.cpp)

add_executable(reliableserver app/ReliableServer.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(reliableserver jsoncpp pthread)
add_executable(reliabletelnet app/ReliableTelnet.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(reliabletelnet jsoncpp pthread)

add_executable(secureserver app/SecureServer.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(secureserver jsoncpp pthread gmp gmpxx crypto)
add_executable(securetelnet app/SecureTelnet.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(securetelnet jsoncpp pthread gmp gmpxx crypto)

add_executable(appserver app/AppServer.cpp src/AppSocket.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(appserver jsoncpp pthread gmp gmpxx crypto)
add_executable(appclient app/AppClient.cpp src/AppSocket.cpp src/SecureSocket.cpp src/ReliableSocket.cpp src/UdpSocket.cpp)
target_link_libraries(appclient jsoncpp pthread gmp gmpxx crypto)
