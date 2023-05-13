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

+ [x] 套接字
+ [ ] 协议解析
+ [ ] 不阻塞 Poll
+ [ ] 实现: 获取 / 插入 / 删除
+ [ ] 组合 [HashTable](https://github.com/akerdi/build-your-own-hash-table)
+ 更多
