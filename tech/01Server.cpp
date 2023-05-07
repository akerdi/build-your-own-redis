//
// Created by shaohong.jiang on 5/6/2023.
//

#include <iostream>
using namespace std;

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>

static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

void start_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv = ::bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv < 0) {
        die("bind()");
    }
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }
    cout << "Listening at port: " << port << endl;

    while (true) {
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