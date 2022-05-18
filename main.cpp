#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <unistd.h>

using namespace  std;

static DBusConnection *connect;

const char * object_path = "/com/jimi/dbus";
const char * signal_object_path = "/signal/jimi/dbus";
const char * method_interface = "ipc.method";
const char * signal_interface = "ipc.signal";
void handle_unregister(DBusConnection * connection, void * user_data);
DBusHandlerResult message_handler(DBusConnection * connection,DBusMessage * msg,void * user_data);
int dbus_send_signal(DBusConnection * connection,const char *signalName, char *value);

static DBusObjectPathVTable server_path_vt = {
    .unregister_function = handle_unregister,
    .message_function = message_handler
};

typedef struct {
    dbus_uint32_t id;
}ConnectData;

ConnectData server_data = {0};

/*读取消息的参数，并且返回两个参数，一个是bool值stat，一个是整数level*/
void reply_to_method_call(DBusMessage * msg, DBusConnection * conn){
    DBusMessage * reply;
    DBusMessageIter arg;
    char * param = NULL;
    dbus_bool_t stat = TRUE;
    dbus_uint32_t level = 2010;
    dbus_uint32_t serial = 0;

    //从msg中读取参数，这个在上一次学习中学过
    if(!dbus_message_iter_init(msg,&arg))
        printf("Message has no args/n");
    else if(dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_STRING)
        printf("Arg is not string!/n");
    else
        dbus_message_iter_get_basic(&arg,&param);
        printf("%s",param);
    if(param == NULL) return;

    string str_reply = param;
    //创建返回消息reply
    reply = dbus_message_new_method_return(msg);
    //在返回消息中填入两个参数，和信号加入参数的方式是一样的。这次我们将加入两个参数。
    dbus_message_iter_init_append(reply,&arg);
    if(!dbus_message_iter_append_basic (&arg,DBUS_TYPE_BOOLEAN,&stat)){
        printf("Out of Memory!/n");
        exit(1);
    }
    if(!dbus_message_iter_append_basic (&arg,DBUS_TYPE_UINT32,&level)){
        printf("Out of Memory!/n");
        exit(1);
    }
    if(!dbus_message_iter_append_basic (&arg,DBUS_TYPE_STRING,&str_reply)){
        printf("Out of Memory!/n");
        exit(1);
    }
  //发送返回消息
      if( !dbus_connection_send (conn, reply, &serial)){
        printf("Out of Memory/n");
        exit(1);
    }
    dbus_connection_flush (conn);
    dbus_message_unref (reply);
}

void reply_to_introspec(DBusMessage * msg, DBusConnection * conn){
    DBusMessage * reply;
    DBusMessageIter arg;

    dbus_uint32_t serial = 0;

    string str_reply;

    //读取xml,xml写入了path下的所有接口及方法,d-feet可以查询
    ifstream in("./dbus_test.xml");
    stringstream buffer;
    buffer<<in.rdbuf();
    str_reply = buffer.str();
    printf("%s",str_reply.c_str());
    //创建返回消息reply
    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply,&arg);

    if(!dbus_message_iter_append_basic (&arg,DBUS_TYPE_STRING,&str_reply)){
        printf("Out of Memory!/n");
        exit(1);
    }
  //发送返回消息
      if( !dbus_connection_send (conn, reply, &serial)){
        printf("Out of Memory/n");
        exit(1);
    }
    dbus_connection_flush (conn);
    dbus_message_unref (reply);
}

DBusHandlerResult message_handler(DBusConnection * connection,DBusMessage * msg,void * user_data)
{
    DBusMessageIter arg;
    char * sigvalue;
    if(!msg){
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    switch(dbus_message_get_type(msg)){
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
    {
        if(dbus_message_is_method_call(msg,method_interface,"test_method")){
            //我们这里面先比较了接口名字和方法名字，实际上应当现比较路径
            auto path = dbus_message_get_path (msg);
            if(strcmp(path,object_path) == 0)
            {
                reply_to_method_call(msg, connection);
            }
         }
        else if(dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE,"Introspect")){ //"org.freedesktop.DBus.Introspectable"
            auto path = dbus_message_get_path (msg);
            if(strcmp(path,object_path) == 0)
            {
                reply_to_introspec(msg, connection);
            }
         }
        break;
    }
    case DBUS_MESSAGE_TYPE_SIGNAL:
    {
        if(dbus_message_is_signal(msg, signal_interface, "test_signal")){
            if(!dbus_message_iter_init(msg,&arg))
                fprintf(stderr,"Message Has no Param");
            else if(dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_STRING)
                fprintf(stderr,"Param is not string");
            else
            {
                dbus_message_iter_get_basic(&arg,&sigvalue);
                //信号内容
                printf("get signal content is : %s",sigvalue);
                fflush(stdout);
                //dbus_send_signal(connection, "test_signal", sigvalue);

                reply_to_method_call(msg, connection);
            }
        }
        break;
    }
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    case DBUS_MESSAGE_TYPE_INVALID:
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    default:
        return DBUS_HANDLER_RESULT_HANDLED;
    }

}

void handle_unregister(DBusConnection * connection, void * user_data){
    dbus_free(user_data);
}



void dbus_server()
{
    DBusMessage * msg;
    DBusMessageIter arg;
    DBusConnection * connection;
    DBusError err;
    int ret;
    char * sigvalue;

    dbus_error_init(&err);
    //创建于session D-Bus的连接
    connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if(dbus_error_is_set(&err)){
        fprintf(stderr,"Connection Error %s/n",err.message);
        dbus_error_free(&err);
    }
    if(connection == NULL)
        return;
    //设置一个BUS name：test.wei.dest
    ret = dbus_bus_request_name(connection,"com.jimi.dbus",DBUS_NAME_FLAG_REPLACE_EXISTING,&err);
    if(dbus_error_is_set(&err)){
        fprintf(stderr,"Name Error %s/n",err.message);
        dbus_error_free(&err);
    }
    if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        return;
    ret = dbus_connection_register_object_path(connection,object_path,&server_path_vt,&server_data);
    if(dbus_error_is_set(&err)){
        fprintf(stderr,"Name Error %s/n",err.message);
        dbus_error_free(&err);
    }

    //消息循环
    while(dbus_connection_read_write_dispatch(connection,-1)){

    }

}

/*发送信号*/
int dbus_send_signal(DBusConnection * connection,const char *signalName, char *value)
{
    dbus_uint32_t serial = 0;
    DBusMessage *msg;
    DBusMessageIter args;
    //指定对象,接口,信号名
    msg = dbus_message_new_signal(
                signal_object_path,
                signal_interface,
                signalName
                );
    if(NULL == msg){
        fprintf(stderr, "dbus_message_new_signal error\n");
        return -1;
    }
    //设置信号内容
    dbus_message_iter_init_append(msg, &args);
    if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,&value)){
        fprintf(stderr, "dbus_message_iter_append_basic error\n");
        return -1;
    }
    //发送信号
    if(!dbus_connection_send(connection, msg, &serial))
    {
        fprintf(stderr, "dbus_connection_send error\n");
        return -1;
    }
    printf("dbus_send_signal: %s-%s\n",signalName, value);
    dbus_connection_flush(connection);

    //释放内存
    dbus_message_unref(msg);
    return 0;
}

int main()
{
    dbus_server();

    return 0;
}
