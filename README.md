# README

## 协议简述

## 项目结构 可执行文件

### `udpTelnet`

一个包含 `main` 函数的可执行文件，其使用方式为：

```bash
$ ./udptelnet <Server IP> <Server Port>
```

之后在标准输入中对程序进行输入，程序会将字符串发送给目标主机。

## 项目结构 类

### `UDPSocket`

一个包含实现 UDP 套接字的类。参考：http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/