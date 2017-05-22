# FSM
----

## 客户端向服务器注册
  * server维护一個名为client_reg的fifo
  * server每次轮询都要先检查client_reg是否有进程加入
  * client进程启动时在发送client_reg 注册请求
  * client 结束时登出, server将其从注册表中剔除

  #### 这种使用管道的实现方式容易被攻击，后续会考虑改用socket


## 通用消息头 msg_t
- 消息头结构:用于消息路由。类比tcp/IP的IP：port，IP和port能够唯一确定一个主机（IP）和应用程序(port)。msg_t中的mdl和pid唯一确定了某类模块集群（mdl）中的某个模块（pid）。
- s_pid: 发送者的pid
- r_pid: 接收者的pid. 发送者在发送消息时有时并不知道目标进程的pid，可将r_pid置`-1`,这样的话server将通过r_mdl选取一个指定类型的模块（进程）.
- s_mdl: 发送者的模块类型(如TTU), 必须在fsm_base.h中定义.
- r_mdl: 接受这的模块类型，必须在fsm_base.h中定义.


## 定时器
* 使用timerfd和epoll实现定时器
* 定时器接口
  - 创建一个定时器：`int gen_real_timer(time_t sec, int isloop)`
  - 启动一个定时器：`int start_timer(int timerid, fsm_t fsmid, time_t seconds)`
  - 终止一个定时器：`void stop_timer(int timerfd)`
  - 搜索定时器：    `fsm_timer* get_timer(int fd)`
* 原理简述：<br>
  start_timer的时候，系统做了一下3件事：
    - 系统将创建一个定时器t
    - 将t的有关信息（定时器文件描述符timerfd,定时器所属状态机fsmid,自动机标识timerid）存储在一个名为timer_dict的字典（主键为timerfd）
    - 将timerfd加入epoll轮询。


## 日志系统
- 相关文件debug.c debug.h [clog.h](https://github.com/mmueller/clog) <br>
- 添加了如下几个宏:<br>
  `LOG_E, LOG_NE, LOG_D, LOG_ND`<br>
  当要打印的日志不带参数时，使用带`N`的宏LOG_NE，LOG_ND，带参数的使用LOG_E，LOG_D。
- 约束<br>
  - 日志句柄 G_LOGGER。G_LOGGER是全局的(const int类型)，最好在main.c文件中定义，并且main.c文件中include "clog.h"之前 define CLOG_MAIN（有且只能一次）。除main.c之外的地方使用日志系统只需要include "debug.h"就可以了。

## C 实现继承和多态
- fsm_entity_base 作为所有fsm的基础(基类) (参照fsm.h fsm.c)

      #define FSM_ENTITY_BASE \
        fsm_constructor constructor;\
        fsm_destructor destructor;\
        fsm_event event;\
        fsm_nextjump_func nextjump;\
        fsm_exception exception;\
        fsm_t fsmid;\
        int timer_list[MAX_ENTITY_TIMER_NUM];\
        char is_fsm_finish;

      typedef struct fsm_entity_base{
          FSM_ENTITY_BASE
      }fsm_entity_base;    

- 构造函数 `constructor`: 每个fsm实体都必须实现constructor，主用用于自动初始化工作。与面向对象特点相同，凡是"继承"fsm_entity_base的状态机都必须调用`fsm_entity_base_constructor`(这是fsm_entity_base的构造函数)。
- 析构函数 `destructor`: 析构函数主要用于释放状态机资源（包括内存资源，定时器等）。fsm_entity_base的析构函数为`fsm_entity_base_destructor`。
-  实例可参考<br>[cachefsm.h](https://github.com/yangluoshen/FSM/blob/master/src/client/demo/hello/cachefsm.h)，[cachefsm.c](https://github.com/yangluoshen/FSM/blob/master/src/client/demo/hello/cachefsm.c)

## fsm (核心)



## 约定
  - client 向其他进程(包括自己)通信时，必须自行组装fsm.h中定义的msg_t

## 关键接口
