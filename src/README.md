FSM
===

1. server如何感知客戶端注冊
----
- server维护一個名为client_reg的fifo
- server每次轮询都要先检查client_reg是否有多的进程加入
- client进程启动时在client_reg 注册
- client 结束时登出, server将其从队列中剔除


2. 添加server日志
----
- 依赖于clog
- 添加了如下几个宏
  - LOG_E, LOG_NE, LOG_D, LOG_ND


3. 添加客户端模板
---
- 目的
  - 方便创建客户端.
  - 客户端只需要在模板上修改即可.
  - 使用者可以只专注与业务逻辑.

4. msg_t
----
- 消息头结构:用于路由
- s_pid: 发送者的pid
- r_pid: 接受者的pid. 发送这在发送消息时有时并不知道目标进程的pid，可将r_pid置-1,这样的话server将通过r_mdl选取一个进程.
- s_mdl: 发送者的模块类型, 必须在fsm.h中定义.
- r_mdl: 接受这的模块类型，必须在fsm.h中定义.

5. timer
----
- 使用timerfd和epoll实现定时器



约定
===
- client 向其他进程(包括自己)通信时，必须自行组装fsm.h中定义的msg_t


