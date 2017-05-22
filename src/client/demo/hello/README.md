# Hello FSM
-----

`hello fsm`模拟两个模块（ttu,yau）同步消息应答过程。
## Usage
- 编译server,ttu,yau
  分别前往`src/server/`,`src/client/demo/hello/TTU`, `src/client/demo/hello/YAU`
  执行make， 得到server,ttu,yau
- 运行<br>
  顺序：先server, 再ttu, 最后yau
- 结果
  - 运行ttu和yau时，服务器接收到客户端请求，将会打印两条注册成功的消息到控制台。
  - 运行yau后：

                  yau say "Nice to meet you"
           yau   ----------------------------------->  ttu
            |     ttu answer "Nice to meet you, too"    |
            |    <----------------------------------    |
            |     yau say "How are you?"                |
            |    ---------------------------------->    |
            |     ttu answer "Find, thanks"             |
            |    <---------------------------------     |

- 日志<br>
  可在/tmp目录下查看进程日志。fsm_cl_M0_xxxx.log是ttu的日志，fsm_cl_M1_xxx.log是yau的日志。
