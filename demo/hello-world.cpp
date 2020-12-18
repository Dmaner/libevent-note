#include <sys/signal.h>
#include <event.h>

// 信号回调函数
void signal_cb(int fd, short event, void* argc)
{
    event_base* base = (event_base*) argc;
    timeval delay = {2, 0};
    printf("\nCaught an interrupt signal\n");
    event_base_loopexit(base, &delay);
}

// 定时器回调函数
void timeout_cb(int fd, short event, void* argc)
{
    printf("timeout\n");
}

int main(int argc, char const *argv[])
{
    // 创建Reactor实例
    event_base* base = event_init();
    
    // 创建信号事件处理器
    event* signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    
    // 将事件处理器添加到事件注册队列中, 并将该事件的对应的事件
    // 添加到相应事件多路分发器中
    event_add(signal_event, NULL);

    // 创建定时器事件处理器
    timeval tv = {1, 0};
    event* timeout_event = evtimer_new(base, timeout_cb, NULL);
    event_add(timeout_event, &tv);
    
    // 执行事件循环
    event_base_dispatch(base);
    
    // 释放资源
    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);

    return 0;
}
