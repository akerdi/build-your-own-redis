# Start socket

本章主要是实现Socket连接，根据`man socket`查看到socket 原型为: `int(*socket)(int domain, int type, int protocol)`.

我们选用IPv4的domain`AF_INET`；以及TCP的type`SOCK_STREAM`；protocol为保留值，使用0即可。

```c++
#include <socket.h>
void start_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");        
    }
    // `setsockopt` 设置fd为可重用地址，防止程序关闭后重启发现端口被占用。
    int val = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));
    // 以下为设置协议簇、端口、地址
    // 协议簇使用AF_INET 指明为IPv4
    // 端口需要将端口参数转为网络字节序
    // addr 使用INADDR_ANY，等同0.0.0.0
  
    // struct sockaddr_in 为现代的结构所以`bind`方法需要强制为`struct sockaddr*)`
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv = ::bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv < 0) {
        die("bind()");
    }
    // 最后只需要listen即可
    rv = listen(fd, SOMAXCONN);
    if (rv < 0) {
        die("listen()");
    }
    cout << "Listening at port: " << port << endl;
    // 对接客户端逻辑
    ...
```

以上开启了服务，成功的话会打印开启的端口。

对接连接，当前使用阻塞的写法，并且接收到内容时即回复:

```c++
void start_server(int port) {
    ...
    // 开启循环接受连接
    while (true) {
        // 连接同socket绑定相同
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = ::accept(fd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd < 0) {
            continue;
        }
        do_something(connfd);
        close(connfd);
    }    
}
```

不同于Windows socket是SOCKET 结构，在Linux系统中，socket 同其他文件描述符一样获得都是`int`类型的句柄。所以关闭时Linux 调用`close`，而Windows 调用`closesocket`。

接下来实现`void(*do_something)(int connfd)`，模拟实现读取和发送:

```c++
void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) -1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);
    
    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}
```

读取函数可以使用read，也可以使用recv: `ssize_t recv(int socket, void* buffer, size_t length, int flags)`.


 