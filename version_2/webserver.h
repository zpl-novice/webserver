#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<iostream>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<string.h>
#include<mysql/mysql.h>
#include"threadpool.h"

using namespace std;

class webserver
{
    public:
	webserver(int port);   //构造函数
	void init();                //初始化
	void web_read(int sockfd);  //读成员函数
        void web_write(int sockfd);  //写成员函数
        void web_accept(int lfd);  //接受连接成员函数
        void reactor();            //事件派发
	static void *callback1(void *);
	~webserver();         //析构函数
	void setp();    //设置当前对象为回调函数使用的对象
    private:
	int lfd;
	int cfd;
	int opt;
	int efd;
	int sockfd;
	int flag;
	int n;
      int num;
	int action;
	char buf[BUFSIZ];
	char request[100];
	int serv_port;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clie_addr;
	socklen_t clie_addr_len;
	struct epoll_event evt;
	struct epoll_event ep[3000];
	MYSQL *mysql;
	threadpool_t *thp;
	static webserver *p;  //存储回调函数调用的对象
};
#endif
