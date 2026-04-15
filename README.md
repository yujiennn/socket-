# socket-

这个仓库用于学习 Linux 下的 TCP 套接字通信，以及一个基于 pthread 的简易线程池。

## 项目内容

- `server.c`：TCP 服务器，接收连接并将通信任务交给线程池处理
- `client.c`：TCP 客户端，连接服务器并进行回显测试
- `threadpool.c` / `threadpool.h`：线程池实现与接口声明

## 环境要求

- Linux
- GCC
- pthread 库（通常系统自带）

## 编译方式

在项目根目录执行：

```bash
gcc server.c threadpool.c -o server -pthread
gcc client.c -o client -pthread
```

## 运行方式

1. 先启动服务端：

```bash
./server
```

2. 再启动客户端：

```bash
./client
```

## 默认配置

- 服务端监听端口：`9999`
- 客户端目标 IP：在 `client.c` 中通过 `inet_pton` 指定，请按实际环境修改

## 目录结构

```text
.
├── client.c
├── server.c
├── threadpool.c
├── threadpool.h
└── README.md
```

## 学习重点

- 基础 TCP 通信流程：`socket -> bind -> listen -> accept -> recv/send`
- 线程池核心机制：任务队列、条件变量、工作线程、管理线程动态扩缩容
- 多线程同步：互斥锁与条件变量配合使用
