#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<iostream>
#include<mysql/mysql.h>
#include"webserver.h"
using namespace std;


int main()
{
    int flag=chdir("/root/文档/webserver");
    if(flag==-1)
    {
	printf("chdir error");
	exit(1);
    }     
    cout<<"change dir"<<endl;
    webserver Webserver(9000);
    Webserver.init();
    cout<<"webserver init"<<endl;
    Webserver.reactor();
    return 0;
}
