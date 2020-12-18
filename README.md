# Libevent Reading Notes

> Libevent源码阅读和添加中文注释  

**目录结构**

```shell
// libevent 2.1.12
// * 代表核心源码
.
├── README.md
├── arc4random.c
├── buffer.c                        // buffer*.c 缓冲区相关函数      
├── buffer_iocp.c
├── bufferevent-internal.h
├── bufferevent.c
├── bufferevent_async.c
├── bufferevent_filter.c
├── bufferevent_mbedtls.c
├── bufferevent_openssl.c
├── bufferevent_pair.c
├── bufferevent_ratelim.c
├── bufferevent_sock.c
├── bufferevent_ssl.c
├── changelist-internal.h
├── compat
│   └── sys
│       └── queue.h                 // 通用数据结构
├── defer-internal.h
├── demo                            // 存放我的一些使用示例
│   ...
├── devpoll.c                       // /dev/poll
├── epoll.c                         // I/O复用: epoll
├── epoll_sub.c
├── epolltable-internal.h
├── evbuffer-internal.h
├── evconfig-private.h.in
├── evdns.c                         // DNS协议相关函数
├── * event-internal.h
├── * event.c                       // 核心代码
├── event_iocp.c
├── event_tagging.c                 // 添加和读取缓冲区中标记数据
├── evmap-internal.h
├── * evmap.c                         // 维护句柄和处理器的关系
├── evport.c                        // I/O复用events poll
├── evrpc-internal.h
├── evrpc.c
├── evsignal-internal.h             // evthread*.c 支持线程相关函数
├── evthread-internal.h
├── evthread.c
├── evthread_pthread.c
├── evthread_win32.c
├── evutil.c
├── evutil_rand.c
├── evutil_time.c
├── file-structure.txt
├── ht-internal.h
├── http-internal.h
├── http.c                          // http协议相关支持
├── include                         
│   ├── evdns.h
│   ├── event.h
│   ├── event2                      // libevent 主要头文件目录
│   │   ├── buffer.h
│   │   ├── buffer_compat.h
│   │   ├── bufferevent.h
│   │   ├── bufferevent_compat.h
│   │   ├── bufferevent_ssl.h
│   │   ├── bufferevent_struct.h
│   │   ├── dns.h
│   │   ├── dns_compat.h
│   │   ├── dns_struct.h
│   │   ├── event.h                 // 核心函数
│   │   ├── event_compat.h
│   │   ├── * event_struct.h
│   │   ├── http.h                  // HTTP协议相关函数
│   │   ├── http_compat.h
│   │   ├── http_struct.h
│   │   ├── keyvalq_struct.h
│   │   ├── listener.h              // 封装socket相关操作
│   │   ├── rpc.h                   
│   │   ├── rpc_compat.h
│   │   ├── rpc_struct.h
│   │   ├── tag.h
│   │   ├── tag_compat.h
│   │   ├── thread.h
│   │   ├── util.h
│   │   ├── visibility.h
│   │   └── watch.h
│   ├── evhttp.h
│   ├── evrpc.h                     // RPC协议相关支持
│   ├── evutil.h
│   └── include.am
├── iocp-internal.h
├── ipv6-internal.h
├── kqueue-internal.h
├── kqueue.c                        // I/O复用: kqueue
├── listener.c
├── log-internal.h
├── log.c                           // 日志相关函数
├── mbedtls-compat.h
├── minheap-internal.h              // 时间堆实现
├── mm-internal.h
├── openssl-compat.h
├── poll.c                          // poll
├── ratelim-internal.h
├── sample                          // 示例代码
│   ├── becat.c
│   ...
├── select.c                        // POSIX select
├── signal.c                        // 信号相关函数
├── ssl-compat.h
├── strlcpy-internal.h
├── strlcpy.c
├── test                            // 测试代码
│   ├── bench.c
│   ...
├── time-internal.h
├── util-internal.h
├── watch.c
├── wepoll.c
├── wepoll.h
└── win32select.c                   // windows select

7 directories, 168 files

```