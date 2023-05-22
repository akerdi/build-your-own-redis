# 操作 set/get/del

上一章中我们已经支持了事件循环，接下来我们要增加操作指令。

本章简单map库当作内存数据存放库。主要精力放在新的协议和处理协议上。

## 新的协议

之前的简单协议已经不够用了，我们想要增加如 `set` `get` `del`, 我们将旧协议改为:

```
+--------+------------+--------+------+--------+------+-----+--------+------+
| len(4) | num str(4) | len(4) | str1 | len(4) | str2 | ... | len(4) | strn |
+--------+------------+--------+------+--------+------+-----+--------+------+
```

`ntr` 本次收到Buffer总大小，`len`为接下来的字符长度。他们都是4byte大小。

返回格式为:

```
+--------+---------------+-----+
| len(4) | status code(4)| msg |
+--------+---------------+-----+
```

## 处理请求

处理请求，将对应数据读取后写入到发送buffer中:

```diff
+static int32_t do_request(
+   const uint8_t* req, uint32_t reqlen,
+   uint32_t* rescode, uint8_t* res, uint32_t* reslen
+) {
+   vector<string> cmds;
+   if (0 != parse_req(req, reqlen, cmds)) {
+       msg("bad req");
+       return -1;
+   }
+   if (cmds.size() == 2 && cmd_is(cmds[0], "get")) {
+       *rescode = do_get(cmds, res, reslen);
+   } else if (cmds.size() == 3 && cmd_is(cmds[0], "set")) {
+       *rescode = do_set(cmds, res, reslen);
+   } else if (cmds.size() == 2 && cmd_is(cmds[0], "del")) {
+       *rescode = do_del(cmds, res, reslen);
+   } else {
+       *rescode = RES_ERR;
+       const char* msg = "Unknown cmd";
+       strcpy((char*)res, msg);
+       *reslen = strlen(msg);
+       return 0;
+   }
+   return 0;
+}
static bool try_one_request(Conn* conn) {
  ...
- printf("client says: %.*s\n", len, &conn->rbuf[4]);
- memcpy(&conn->wbuf[0], &len, 4);
- memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
- conn->wbuf_size = 4 + len;
+ uint32_t rescode = 0;
+ uint32_t wlen = 0;
+ int32_t err = do_request(
+     &conn->rbuf[4], len,
+     &rescode, &conn->wbuf[4+4], &wlen
+ );
+ if (err) {
+     conn->state == STATE_END;
+     return false;
+ }
+ wlen += 4;
+ memcpy(&conn->wbuf[0], &wlen, 4);
+ memcpy(&conn->wbuf[4], &rescode, 4);
+ conn->wbuf_size = 4 + wlen;
}
```

`try_one_request` 通过调用`int32_t(*do_request)(const uint8_t* req, uint32_t reqlen, uint32_t* rescode, uint8_t* res, uint32_t* reslen)` 执行动作并将结果写入res和rescode中。

函数`do_request` 先通过`int32_t(*parse_req)(void* req, size_t reqlen, vector<string>& cmd)`解析出请求的执行类型:

```c++
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
```

上面可以看到，最开始获取到字符串个数n。

通过while 循环不断拿出字符长度，和对应长度的字符，并写入到`out`的vector容器中。

获取到对应的`cmds`后，即可执行`get` / `set` / `del`.