# 创建自己的redis | Build your own redis

如何制作一个Redis，翻译文章自[build-your-own/redis](https://build-your-own.org/redis).

目前实现 1~8，其中第8章Hashtable 直接采用[HashTable教程](https://github.com/akerdi/build-your-own-hash-table), 原因是原教程未能快速理解。

> 关于Socket课程，推荐初学者优先学习课程[tinyhttpd](https://github.com/akerdi/tinyhttpd)

## 环境 | Environment

```
开发工具(IDE) :      CLion
语言(Language):     C++ 11
平台(Platform):     Linux/Mac
```

## 目标 | Goal

+ [x] 套接字 Socket
+ [ ] 协议分解 | Protocol Parsing
+ [ ]  Nonblock Poll
+ [ ] Implement: get / insert / del
+ [ ] Integrate [HashTable](https://github.com/akerdi/build-your-own-hash-table)
+ More on...
