# 使用FSM模拟“哲学家就餐问题”

## Usage
建议一下几个操作分别在三个控制终端执行
  - tty1 :编译并启动服务器
    `cd FSM/src/server/; make; ./server`
  - tty2: 编译并启动 phu
    `cd FSM/src/client/demo/philosopher;make phu; ./phu`
  - tty3: 编译并启动 cru
    `cd FSM/src/client/demo/philosopher;make cru; ./cru 6`

短暂等待之后, 在`tty2` 将会实时显示哲学家的“思考”和“就餐”状态。

## 依赖
   为方便显示，需要libncurses.so支持。
## 场景描述
<p>一个圆桌围着n个只会“Think”和"eat“的哲学家，每个哲学家两边各有一根筷子，由于哲学家围着桌子，所以一共有n根筷子。每个哲学家的eat用时和think用时可能都不一样。哲学家的初始状态为”think“，”think"饿了便会先看下左手边的筷子是否可用，如果不可用继续think；否则看下右手边的筷子，如果不可用，放下左手边的筷子并继续think；若可用，便可以用餐啦！哲学家用完餐会释放所用的筷子，并开始think...<br>
哲学家获得筷子之后都会在一定时间之后归还。<br> 
与原来的哲学家就餐问题不同，这种机制下，理论上不会造成死锁。</p>

## cru
cru 用于创建哲学家。cru启动时接受一个参数num，即哲学家人数（2-8）。
cru启动后发送创建哲学家的请求(PHILOS_CREATE_REQ)给phu，phu根据消息内容创建一个哲学家（状态机），然后该哲学家便开始“思考”和“就餐”了。
