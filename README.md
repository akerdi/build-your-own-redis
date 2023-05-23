# 实现Redis

[English README](./README-eng.md)

如何制作一个Redis，翻译文章自[build-your-own/redis](https://build-your-own.org/redis).

目前实现 1~8，其中第8章HashTable 直接采用[HashTable教程](https://github.com/akerdi/build-your-own-hash-table), 原因是原教程未能快速理解。

> 关于Socket课程，推荐初学者优先学习课程[tinyhttpd](https://github.com/akerdi/tinyhttpd).

## 环境

```
开发工具:   CLion
语言:       C++11
平台:       Linux/Mac
```

## 目标

+ [x] [套接字](./tech/01Server.md)
+ [x] [协议解析](./tech/02Protocol.md)
+ [x] [非阻塞](./tech/03NonBlock.md)
+ [x] [实现: 获取 / 插入 / 删除](./tech/04Operate.md)
+ [x] 组合 [HashTable](https://github.com/akerdi/build-your-own-hash-table)
+ 更多，敬请期待...
