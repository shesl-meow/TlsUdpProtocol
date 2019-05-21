# README

## 运行环境

以下为编译本项目的一些说明:

1. 本项目引用了 `jsoncpp` 这个项目，在 `Linux` 环境下，安装 `jsoncpp`，编译时使用命令行参数 `-ljsoncpp`. 参考：<https://www.codeproject.com/Articles/1102603/Accessing-JSON-Data-with-Cplusplus>

## 协议简述

### `ReliableSocket`

这个简单的 `TlsUdpProtocol` 项目在实现可靠的传输层协议 `ReliableSocket` 时忽略了以下这些问题：

1. **不考虑**服务器同时为**多个客户端**提供服务的情况，客户端连接之后不会进行线程新增、分配资源等工作；
2. 因为不分配资源，在实现可靠的 `UDP` 传输时，双方**不进行三次握手**交换初始序列号，默认初始序列号为 0，只进行一次客户端告知长度的握手过程；
3. 协议会对信息进行分包，**包的默认大小为 `1K`** (1024 bytes)；
4. 协议**不实现窗口机制**，将按序号简单传输所有块，并且等待每个块的 `ACK`；
5. 协议不实现快速重传，**只实现超时重传**，默认的 `timoutInterval` 为 `1s`；

`ReliableSocket` 建立连接的过程：

![ReliableSocket](./ReliableSocket.svg)

使用这个类库只需要，调用 `setPackets`, `sendPackets`，`startListen` 这三个函数即可。

### `SecureSocket`

安全层建立连接:

```mermaid
sequenceDiagram

participant 服务端
participant 客户端

客户端->>服务端: 
```

## 项目结构 可执行文件

### `TestClass/UdpTelnet.cpp`

一个包含 `main` 函数的可执行文件，其使用方式为：

```bash
$ ./udptelnet <Server IP> <Server Port>
```

之后在标准输入中对程序进行输入，程序会将字符串发送给目标主机。

## 项目结构 类

### `UdpSocket`

一个包含实现 UDP 套接字的类。参考：http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/