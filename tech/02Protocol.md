# 协议解析

上一课中，每次请求都只有一次响应，随后关闭该连接。

如果一个连接首次加载，需要获取服务端多个内容或者多个数据，每次获取的数据都会断开此次连接，那么该服务首次可使用的等待时间就等于所有请求时间+回复时间的总和。

这是不可接受的，对于不能在200ms内打开的服务，我们都会找更好的替代品。

本章通过某一个连接内循环判断是否还有内容，来确定是否关闭当前socket，期间及时回复当前内容。

## 选择传输协议

为了对接不同客户端，我们需要实现一个自定义协议。比如参考HTTP body 的`Content-Length:` 读取body体长度，我们设置个简单的协议:

```
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------
```

这个协议由两部分组成，一个4-byte 的`小端`整数，表明接下来的内容大小。

本章节同样还是阻塞Socket api，一次连接后处理该次连接所有发送字节:

```diff
void start_server(int port) {
    ...
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = ::accept(fd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd < 0) {
            continue;
        }
-       do_something(connfd);
+       while (true) {
+           int32_t err = one_request(connfd);
+           if (err) {
+               break;
+           }
+       }
        close(connfd);
    }
}
```

## 控制IO

以下实现读、写帮助函数:

```c++
static int32_t read_full(int fd, char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}
static int32_t write_all(int fd, const* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}
```

注意, 一次请求中，read得到的数据不一定是全的，比如你想发送512 bytes，但是该次仅接收到412。因为Socket 基于kernel进行转发，但是kernel该次设定仅发送特定长度。[注1](https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html#sendall)。所以通过read_full 函数帮助读取该次想要读取到的大小。值得注意的是，当获取到的`rv <= 0`时，说明已无内容或者客户端断开行为。

写函数也是同理。

## 解析协议

函数`one_request` 负责解析，第一次直接获取大小为4字节`uint32_t len` 参数，然后再次获取大小为`len`字节参数:

```c++
const size_t k_max_msg = 4096;
static int32_t one_request(int connfd) {
    char rbuf[4+k_max_msg+1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    rbuf[4+len] = '\0';

    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}
```

大小端曾经都需要被考虑进去，在这里我们简单认为都是小端。

以上实现了服务端的阻塞多次接受数据行为，并为此实现了一个简单的通信协议。
