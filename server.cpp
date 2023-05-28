//
// Created by shaohong.jiang on 5/6/2023.
//

#include <iostream>
#include <vector>
using namespace std;

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>

#include <HashTable.hpp>

const size_t k_max_msg = 4096;
const size_t k_max_args = 1024;
static HashTable<string>* g_map;

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2, // mark the connection for deletion
};
enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

struct Conn {
    int fd = -1;
    uint32_t state = 0;

    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4+k_max_msg];
};

static void fd_set_nb(int fd);
static void die(const char* msg);
static void msg(const char* msg);
static void state_req(Conn *conn);
static void state_res(Conn *conn);

static void conn_put(vector<Conn*>& fd2conn, Conn* conn) {
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}
static int32_t accept_new_conn(vector<Conn*>& fd2conn, int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = ::accept(fd, (struct sockaddr*)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1; // error
    }
    fd_set_nb(connfd);

    Conn* conn = (Conn*)malloc(sizeof(Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}

static int32_t parse_req(
    const uint8_t* data, size_t len, vector<string>& cmds
) {
    if (len < 4) return -1;

    int32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > k_max_args) return -1;

    size_t pos = 4;
    while (n--) {
        if (pos + 4 > len) {
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len) {
            return -1;
        }
        cmds.push_back(string((char*)&data[pos+4], sz));
        pos += 4 + sz;
    }
    if (pos != len) {
        return -1;
    }
    return 0;
}

static uint32_t do_get(
    const vector<string>& cmds, uint8_t* res, uint32_t* reslen
) {
    if (!g_map->ht_search(cmds[1].c_str())) {
        return RES_NX;
    }
    string* val = g_map->ht_search(cmds[1].c_str());
    uint32_t val_size = sizeof(val);
    assert(val_size <= k_max_msg);
    memcpy(res, val, val_size);
    *reslen = val_size;
    return RES_OK;
}
static uint32_t do_set(
    const vector<string>& cmds, uint8_t* res, uint32_t* reslen
) {
    g_map->ht_insert(cmds[1].c_str(), cmds[2].c_str());
    return RES_OK;
}
static uint32_t do_del(
    const vector<string>& cmds, uint8_t* res, uint32_t* reslen
) {
    g_map->ht_del(cmds[1].c_str());
    return RES_OK;
}

static bool cmd_is(const string& word, const char* cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

static int32_t do_request(
    const uint8_t* req, uint32_t reqlen,
    uint32_t* rescode, uint8_t* res, uint32_t* reslen
) {
    vector<string> cmds;
    if (0 != parse_req(req, reqlen, cmds)) {
        msg("bad req");
        return -1;
    }
    if (cmds.size() == 2 && cmd_is(cmds[0], "get")) {
        *rescode = do_get(cmds, res, reslen);
    } else if (cmds.size() == 3 && cmd_is(cmds[0], "set")) {
        *rescode = do_set(cmds, res, reslen);
    } else if (cmds.size() == 2 && cmd_is(cmds[0], "del")) {
        *rescode = do_del(cmds, res, reslen);
    } else {
        *rescode = RES_ERR;
        const char* msg = "Unknown cmd";
        strcpy((char*)res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}

static bool try_one_request(Conn* conn) {
    if (conn->rbuf_size < 4) {
        // not enough data in the buffer. Will retry in the next iteration
        // maybe next iteration will add more buffer
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state == STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        // not enough data in the buffer. Will retry in the next iteration
        // maybe next iteration will add more buffer
        return false;
    }

    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(
        &conn->rbuf[4], len,
        &rescode, &conn->wbuf[4+4], &wlen
    );
    if (err) {
        conn->state == STATE_END;
        return false;
    }
    wlen += 4;
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;

    // too much buffer ? leave it be to the next iteration to handle with it
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;
    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

static bool try_fill_buffer(Conn* conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }
    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    while (try_one_request(conn)) {}
    return conn->state == STATE_REQ;
}

static void state_req(Conn* conn) {
    while (try_fill_buffer(conn)) {}
}
static bool try_flush_buffer(Conn* conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    // EAGAIN means try again, no buffer now
    // Stop when meet errno equals to EAGAIN
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        // response was fully sent, change state back to req, try REQ again
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    return true;
}

static void state_res(Conn* conn) {
    while (try_flush_buffer(conn)) {}
}

static void connection_io(Conn* conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0); // not expected
    }
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
    fd_set_nb(fd);
    cout << "Listening at port: " << port << endl;

    g_map = &HashTable<string>::ht_new();

    vector<Conn*> fd2conn;
    vector<struct pollfd> poll_args;
    while (true) {
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (Conn* conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events |= POLLERR;
            poll_args.push_back(pfd);
        }

        int rv = poll(poll_args.data(), poll_args.size(), 1000);
        if (rv < 0) {
            die("poll");
        }

        for (size_t i = 1; i < poll_args.size(); i++) {
            if (poll_args[i].revents) {
                Conn* conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    fd2conn[conn->fd] = NULL;
                    close(conn->fd);
                    free(conn);
                }
            }
        }

        if (poll_args[0].revents) {
            accept_new_conn(fd2conn, fd);
        }
    }
}

static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}
static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}
