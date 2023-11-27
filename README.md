# mq——mac下安装、使用

## 安装

https://blog.csdn.net/keysilence1/article/details/80264882

brew install rabbitmq安装 

安装目录在/usr/local/Cellar/rabbitmq/3.8.2/下，以root权限启动 sudo sbin/rabbitmq-server启动

输入http://localhost:15672/  即可进入mq，默认用户名和密码是guest

## 简单使用

进入sbin文件夹下

创建用户：sudo ./rabbitmqctl add_user bajie 123

设置用户身份：sudo ./rabbitmqctl set_user_tags bajie administrator

分配管理员权限：sudo ./rabbitmqctl set_permissions -p "/" bajie ".*" ".*" ".*"

## c++使用rabbitmq

参考：

​		例子

- https://blog.csdn.net/qq78442761/article/details/93158659 （简单实现）

- https://blog.csdn.net/qq78442761/article/details/94012784 （公平分发）

  api说明

- https://blog.csdn.net/u012861467/article/details/106517031

- https://blog.csdn.net/weixin_44353800/article/details/107733075

- https://blog.csdn.net/Hopoxipo/article/details/103763001

- https://blog.csdn.net/u013946356/article/details/82420489




### 一个生产者对一个消费者

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210330153556.png)

生产者：声明一个队列，将数据amqp_basic_public存入队列中

消费者：根据队列名称用amqp_basic_consum取其中的数据（一大坨的读）

代码实现：Send.cpp，Recv.cpp

出现问题：生产者成功连接服务器，但是无法把消息传输进队列：没有通过amqp_queue_declare声明队列，所以传输的时候找不到相应名字的队列

### 一个生产者对多个消费者：工作队列（多个消费者同时处理，在多个工作人员之间分配耗时的任务）

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210330161732.png)

消息持久性：设置持久队列（生产者端声明队列时设置）

消息确认：设置ack应答（取了一条数据之后，消费者回复确认）

公平分发：amqp_basic_get()去读取RabbitMQ的数据，同时使用amqp_read_message()把数据给读出来。读完一条数据就发一个确认

代码实现：NewTask.cpp，Worker1.cpp，Worker2.cpp

### 生产者把消息发到交换机

https://blog.csdn.net/weixin_42148156/article/details/113445913 （交换机函数）

如果没有队列绑定到交换，则消息将丢失，但这对我们来说是可以的。如果没有消费者在听，我们可以放心地丢弃该消息。

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210331153808.png)

生产者：声明交换机

fanout类型交换机：将接收到的所有消息广播到它知道的所有队列中

`amqp_exchange_declare(connState, 1, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(type.c_str()), **false**, **true**, **false**, **false**, amqp_empty_table);`

消费者：创建随机队列（声明队列的时候把***exclusive***设置为true，不指定队列的名称，由系统随机生成），通过绑定告诉交换机将消息发送到我们的队列

`amqp_queue_declare_ok_t_ *channel_id = amqp_queue_declare(connState, 1, amqp_cstring_bytes(""), **false**, **true**, **true**, **false**, amqp_empty_table);`

`amqp_queue_bind(connState, 1, channel_id->queue, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(""), amqp_empty_table);`

代码实现：EmitLog.cpp，ReceiveLogs.cpp，ReceiveLogs2.cpp（模拟有两个队列）

出现问题：不知道如何设置随机队列，以及生成的随机队列名称如何获取（声明队列后的返回值内包含了随机队列名称）

### 选择性的接收消息

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210407135946.png)

生产者：直接交换的交换机：一条消息进入绑定键与该消息的路由键完全匹配的队列 。

消费者：根据绑定建有选择的接收消息

代码实现：EmitLogDirect.cpp，ReceiveLogsTopic.cpp，ReceiveLogsTopic2.cpp（模拟有两个不同绑定建的队列）

出现问题：生产者绑定键的类型从操作台接收的时候采用字符数组，后续转换为了amqp_bytes_t类型（是指针保存为地址），而消费者端的绑定建类型是字符串类型所以两者绑定建不同，消费者读不到数据：将生产者端通过getlin(cin,s)直接读入字符串类型的绑定建

### 基于模式（主题）接收消息

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210407144146.png)

主题类型的交换机：routing-key必须是单词列表，以点分隔

*（星号）可以代替一个单词。

＃（哈希）可以替代零个或多个单词。

代码实现：EmitLogTopic.cpp，ReceiveLogsDirect.cpp，ReceiveLogsDirect2.cpp（模拟有两个不同绑定建的队列）

### 使用rabbitmq构建rpc系统

https://blog.csdn.net/singleroot/article/details/51547603 （例子参考）

![](https://cdn.jsdelivr.net/gh/iamxpf/pageImage/images/20210407154207.png)

RPC将像这样工作：

- 对于RPC请求，客户端发送一条具有以下两个属性的消息： replyTo（设置为仅为该请求创建的匿名互斥队列）和correlationId（设置为每个请求的唯一值）。
- 该请求被发送到rpc_queue队列。
- RPC工作程序（又名：服务器）正在等待该队列上的请求。出现请求时，它会使用replyTo字段中的队列来完成工作并将带有结果的消息发送回客户端。
- 客户端等待答复队列中的数据。出现消息时，它会检查correlationId属性。如果它与请求中的值匹配，则将响应返回给应用程序。



