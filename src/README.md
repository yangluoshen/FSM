Server
===

1. server如何感知客戶端注冊
----
- server和clients共同维护一個名为client_reg的fifo
- server每次轮询都要先检查client_reg是否有多的进程加入
- client进程启动时在client_reg 注册
- client 结束时登出


2. 添加server日志
