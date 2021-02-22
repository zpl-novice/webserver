#include"webserver.h"

webserver::webserver(int port)
{
    serv_port=port;
}
webserver::~webserver()
{
    close(lfd);
    close(cfd);
    close(efd);
}
void webserver::init()
{
    lfd=socket(AF_INET,SOCK_STREAM,0);  //创建套接字
    if(lfd==-1)
    {
	cout<<"create socket error";
    }
    opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));  //设置端口复用
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(serv_port);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
    flag=bind(lfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));  //绑定
    if(flag==-1)
    {
	cout<<"bind error";
    }
    flag=listen(lfd,120);      //监听
    if(flag==-1)
    {
	cout<<"listen error";
    }
    efd=epoll_create(3000);   //创建epoll红黑树
    if(efd==-1)
    {
	cout<<"create epoll error";
    }
    evt.data.fd=lfd;
    evt.events=EPOLLIN;
    flag=epoll_ctl(efd,EPOLL_CTL_ADD,lfd,&evt);  //将lfd添加到红黑树上
    if(flag==-1)
    {
	cout<<"add epoll error";
    }
}
void webserver::reactor()
{
    for(;;)
    {
	n=epoll_wait(efd,ep,3000,-1);   //循环阻塞监听
	if(n==-1)
        {
	    cout<<"epoll wait error";
        }
        for(int i=0;i<n;i++)     //遍历数组，调用对应事件
        {
	    if(ep[i].data.fd==lfd)
	    {
	        web_accept(lfd);
	    }
	    else
	    {
	        sockfd=ep[i].data.fd;
	        web_read(sockfd);      //调用读函数解析请求消息
	    }
        }
    }
}
void webserver::web_accept(int lfd)     //建立连接，添加到树上
{
    clie_addr_len=sizeof(clie_addr);
    cfd=accept(lfd,(struct sockaddr*)&clie_addr,&clie_addr_len);
    flag=fcntl(cfd,F_GETFL);   //设置为非阻塞读
    flag |= O_NONBLOCK;
    fcntl(cfd,F_SETFL,flag);
    if(cfd==-1)
    {
	cout<<"accept error";
    }
    evt.events=EPOLLIN | EPOLLET;
    evt.data.fd=cfd;
    flag=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&evt);
    if(flag==-1)
    {
	cout<<"add epoll error";
    }    	
}
void webserver::web_read(int sockfd)
{   
    memset(buf,0,sizeof(buf));
    num=read(sockfd,buf,sizeof(buf));
    if(num<0)
    {
	cout<<"read error";
    }
    if(num==0)               //无数据传输，从树上摘除
    {
	flag=epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
	close(sockfd);
    }
    else
    {
	action=20;
	char method[12]={0},path[100]={0},protocol[12]={0},user[20]={0},password[20]={0};
	sscanf(buf,"%[^ ] %[^ ] %[^ \r\n]",method,path,protocol);
	if(strcmp(method,"GET")==0)
	{
	    if(strcmp(path,"/")==0)
	    {
		action=0;          //选择界面
	    }
	    if(strcmp(path,"/1?")==0)
	    {
		action=1;          //注册界面
	    }
	    if(strcmp(path,"/2?")==0)
	    {
		action=2;          //登陆界面
	    }
	    if(strcmp(path,"/mainsceen")==0)
	    {
		action=10;          //主界面
	    }
	    if(strcmp(path,"/11?")==0)
	    {
		action=11;          //图片界面
	    }
	    if(strcmp(path,"/C++.jpg")==0)
	    {
		action=14;          //图片资源
	    }
	    if(strcmp(path,"/15?")==0)
	    {
		action=15;          //图片下载
	    }
	    if(strcmp(path,"/12?")==0)
	    {
		action=12;          //音频界面
	    }
	    if(strcmp(path,"/TheOath.ogg")==0)
	    {
		action=16;          //音频资源
	    }		
	    if(strcmp(path,"/17?")==0)
	    {
		action=17;          //音频下载
	    }	
	    if(strcmp(path,"/13?")==0)
	    {
		action=13;          //文档界面
	    }
	    if(strcmp(path,"/18?")==0)
	    {
		action=18;          //文档下载
	    }
	    if(strcmp(path,"/favicon.ico")==0)
	    {
		action=20;          //图标处理
	    }	    
	}
	if(strcmp(method,"POST")==0)
	{
	    mysql=NULL;
    	    mysql=mysql_init(mysql);
    	    if (mysql == NULL)
    	     {
        		cout << "Error:" << mysql_error(mysql);
        		exit(1);
    	      }
    	    mysql=mysql_real_connect(mysql,"localhost","root","123456","webserver",3306,NULL, 0);                 //连接的数据库名账号密码端口等
    	    if (mysql == NULL)
    	    {
        	   cout << "Error: " << mysql_error(mysql);
        	   exit(1);
    	    }
    	    cout<<"open mysql"<<endl;

	    //匹配post输入账号密码
	    cout<<num<<endl;
	    int m=n=0;
	    for(int j=num-1;j>num-100;j--)   //密码个数
	    {
		if(buf[j]=='=')
		{
		    break;
		}
		m++;
	    }
	    for(int j=num-11-m;j>num-100;j--)   //用户名个数
	    {
		if(buf[j]=='=')
		{
		    break;
		}
		n++;
	    }
	    for(int j=0;j<m;j++)          //密码
	    {
		password[j]=buf[j+num-m];
	    }
	    for(int j=0;j<n;j++)          //用户名
	    {
		user[j]=buf[j+num-10-m-n];
	    }
	    cout<<n<<" "<<m<<endl;
	    cout<<user<<" "<<password<<endl;	    
	    if(strcmp(path,"/3")==0)
	    {
		char bufff[128]="select * from info where user='";   //拼接sql语句在数据库中查询
		strcat(bufff,user);
		strcat(bufff,"' and password='");
		strcat(bufff,password);
		strcat(bufff,"';");
		int res=mysql_query(mysql,bufff);       	
        	MYSQL_RES *result=mysql_store_result(mysql);       //从表中检索完整的结果集
        	int num_rows=mysql_num_rows(result);         //返回结果集中的行数
		cout<<123<<num_rows<<123<<endl;		  
		if(num_rows==1){
			action=10;          //登陆成功后主界面
		}
		else{
			action=6;          //登陆失败界面
		}
	    }
	    if(strcmp(path,"/4")==0)
	    {
		char buff[128]="insert into info values('";   //拼接sql语句在数据库中注册
		strcat(buff,user);
		strcat(buff,"','");
		strcat(buff,password);
		strcat(buff,"');");
		int ret=mysql_query(mysql,buff);       //成功返回0
		cout<<123<<buff<<123<<endl;
		cout<<123<<ret<<123<<endl;
		if(ret==0)
		{
			action=4;          //注册成功跳转到登陆界面
		}
		else{
			action=5;          //注册失败跳转到注册错误界面
		}	    
	    }
	   mysql_close(mysql);	
	}
	cout<<buf<<endl;
	web_write(sockfd);
    }
}
void webserver::web_write(int sockfd)
{
    memset(buf,0,sizeof(buf));
    if(action==10)       //主界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/mainsceen.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==11)  //图片界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/picture.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
   else if(action==12)    //音频界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/voice.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==13)    //文本界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/text.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==14)    //图片资源
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:image/jpeg\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/C++.jpg",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==15)    //图片下载
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:application/octet-stream\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/C++.jpg",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==16)     //音频资源
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:video/ogg\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/TheOath.ogg",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==17)   // //音频下载
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:video/mpeg4\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/TheOath.mp4",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==18)   //文本下载
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:application/octet-stream\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/hello.txt",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==0)   //选择界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/judge.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==1)   //注册界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/register.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==2)   //登陆界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/login.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==4)   //注册成功后跳转到登陆界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/login.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==5)   //注册失败跳转到注册出错界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/registererror.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
    else if(action==6)   //登陆失败界面
     {
	    sprintf(buf,"HTTP/1.1 200 OK\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/html;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    int fd=open("/root/文档/webserver/loginerror.html",O_RDONLY);  //响应正文
	    if(fd==-1)
	    {
	        cout<<"error 404";
	    }
            int len=0;                       //循环读取发送
 	    while((len=read(fd,buf,sizeof(buf)))>0)
	    {
	        send(sockfd,buf,len,0);   //发送
		cout<<buf;
	    }
	    close(fd);
    }
   else{
	    sprintf(buf,"HTTP/1.1 404 NOT FOUND\r\n");  //状态行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"Content-Type:text/plain;charset=UTF-8\r\n");  //消息报头
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"\r\n");  //空行
	    cout<<buf;
	    send(sockfd,buf,strlen(buf),0);   //发送
	    sprintf(buf,"404 NOT FOUND");  //空行
	    send(sockfd,buf,strlen(buf),0);   //发送
   }
   close(sockfd);
   cout<<"发送完毕"<<endl;
}



