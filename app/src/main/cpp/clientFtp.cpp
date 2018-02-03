#include<jni.h>
#include<stdio.h> //for printf
#include<stdlib.h>//for exit
#include<string.h>//for bzero
#include<sys/types.h> //for socket
#include<sys/socket.h>
#include<netinet/in.h>//for  struct sockaddr_in
#include<sys/select.h> //for select
#include<netinet/in.h>//for htons
#include<sys/time.h>//for timeval
#include<arpa/inet.h>//for inet_addr() 将一个点分十进制的IP转换成一个长整数型数
#include<unistd.h>//for socket,for pread();
#include<time.h>
#include<errno.h>//for strerror
#include<sys/stat.h>//for stat
#include<fcntl.h>//for open
#include<pthread.h>
#include <netdb.h>//for getaddrinfo for ipv4/ipv6 ..gai_strerror. 翻译返回值！！！
#include"com_example_myapplication_MainActivity.h"
#include"my_log.h"
#include"clientFtp.h"

int CFTP::DATA_TRY_TIMES  = 3;  //数据套接字失败重试次数
int CFTP::DATA_TRY_INTERVAL  = 2000;//数据套接字失败重试间隔
char *file_path="/storage/emulated/0/easonchen.mp3";
char *PROFILE="/storage/emulated/0/testprofile.conf";


CFTP::CFTP(void)
{
    m_DataSocket = INVALID_SOCKET;
    m_ControlSocket = INVALID_SOCKET;
    IsRecvAlready = 1;
    m_nTimeOut=200;
}
CFTP::~CFTP(void)
{
    CloseDataSocket();
    CloseCtrlSocket();
}
char rbuf[800];
char sbuf[200];

struct hostent *hptr;
//pthread_t tid[4];
typedef struct filepart{
    size_t start;
    size_t size;
}filePart;
filePart* blocks=(filePart*)malloc(sizeof(filePart)*4);
size_t filesize;

int CFTP::SendControl(const char *pszData)
{
     if(IsRecvAlready==0)
     {
        LOGI("thread[%d]:   Last Send not compeleted ！！！sleep（100）",m_nThread);
        sleep(100);
     }
     if(IsRecvAlready==1)
     {
        if( send(m_ControlSocket,pszData,strlen(pszData),0) == SOCKET_ERROR )
        {
            LOGI("thread[%d]: IsRecvAlready：%d，  send error...%s   errno=%d\n  %d",m_nThread,IsRecvAlready,strerror(errno),errno,__LINE__);
            return -1;
        }
        IsRecvAlready=0;
        LOGI("thread[%d]:   %s,%s,LINE==%d",m_nThread,__FUNCTION__,pszData,__LINE__);
        return 1;
     }
     else {
        LOGI("thread[%d]:   Last Send not compeleted ！！！sleep（100）",m_nThread);
        return -1;
     }
}
int CFTP::RecvControl(const char *pszResult)
{
    char m_szBuf[RECV_BUF_LEN];//接收命令回复数组；
    memset(m_szBuf,0,RECV_BUF_LEN);
    if(recv(m_ControlSocket,m_szBuf,RECV_BUF_LEN,0)<0)
    {
        LOGI("thread[%d]:   recv error...%s   errno=%d\n  %d",m_nThread,strerror(errno),errno,__LINE__);
        IsRecvAlready=1;
        return -1;
    }
    LOGI("thread[%d]:   %s,%s,LINE==%d",m_nThread,__FUNCTION__,m_szBuf,__LINE__);
    if(pszResult!=NULL)
    {
        if(!strstr(m_szBuf,pszResult))
        {
            LOGI("thread[%d]:   %s,%s,LINE==%d",m_nThread,__FUNCTION__,m_szBuf,__LINE__);
            IsRecvAlready=1;
            return -1;
        }
    }
    IsRecvAlready=1;
    return 1;
}
int CFTP::SendAndRecv(const char * pszCMD,char * pszResult /*= NULL*/)
{
    LOGI("thread[%d]:pszCMD==%s, pszResult==%s,LINE==%d",m_nThread,pszCMD, pszResult,__LINE__);
	if(SendControl(pszCMD)==-1)
	{
	    LOGI("thread[%d]:   sendcontrol error have done ,but error!!! LINE==%d",m_nThread,__LINE__);
	    return -1;
	}
   // LOGI("thread[%d]: RECV CONTROL START???",m_nThread);
	if(RecvControl(pszResult)==-1)
	{
	    LOGI("thread[%d]:   recvcontrol  have done ,but error!!! LINE==%d",m_nThread,__LINE__);
	    return -1;
	}

	return 1;
}
int CFTP::RecvData(char *pszBuf,int iLength)
{
    int nReturn=0;
    while(iLength>0)
    {
        pszBuf+=nReturn;
        nReturn=recv(m_DataSocket,pszBuf,iLength,0);
        if(nReturn==SOCKET_ERROR)
        {
            return -1;
        }
        if(nReturn==0)
        {
            break;
        }
        iLength-=nReturn;
        if(iLength<=0)
        {
            LOGI("thread[%d]:   RECV_LIST_BUFEER_LENGTH TOO SMALL",m_nThread);
            return -1;
        }
    }
    if(RecvControl("226")==-1)
    return -1;
    CloseDataSocket();
    return 1;
}
int CFTP::SendData(char *pszData,int nlength)
{
    int nSend=0;
    while(nlength>0)
    {
        pszData+=nSend;
        nSend=send(m_DataSocket,pszData,nlength,0);
        if(nSend == SOCKET_ERROR)
        {
            return -1;
        }
        nlength -= nSend;
    }
    return 1;
}
 int  GetLocalFileSize(char * pszFile)
 {
	 if (!pszFile)
		 return -1;
	 FILE *fp = NULL;
	 fp = fopen(pszFile, "rb");
	 if (!fp)
		 return -1;
	 fseek(fp, 0, SEEK_END);
	 int iLength = ftell(fp);
	 fseek(fp, 0, SEEK_SET);
	 fclose(fp);
	 return iLength;
 }

int  CFTP::ReadAndSendPatch(char *pSendPath,int nPatchNum,int nSizeOnFtp)  //第三个参数在ftp上同文件包的大小！！！
{
    LOGI("thread[%d]:   发送数据包 : %d！！！LINE==%d",m_nThread,nPatchNum,__LINE__);
    int RollNumber=0;
    int ret;
    int nFileSize = GetLocalFileSize(file_path);
    LOGI("thread[%d]:   文件大小：%d,LINE==%d",m_nThread,nFileSize,__LINE__);
    int nRead=0;
    int nRollRead=0;
    if(nFileSize <= 0)
    {
        return -1;
    }
    char ReadBuf[4*1024]={0};
    char szErrorBuf[300] = {0};
    int waitToSendSize=nFileSize-nPatchNum*1024*1024;
    if(nSizeOnFtp==waitToSendSize)
    {
        LOGI("文件已经完整存在于ftp，LINE==%d",__LINE__);
        return 1;
    }
    if(waitToSendSize>=patchSize)
    {
        waitToSendSize=patchSize;
    }
    int pFile=open(file_path,O_RDONLY | O_LARGEFILE);
    if(pFile<0)
    {
        LOGI("thread[%d]:   上传文件%s打开失败",m_nThread,file_path);
        return -1;
    }
    off_t file_offset;
    if(nSizeOnFtp>0&&nSizeOnFtp<waitToSendSize)//服务器本来存在此文件包，断点续传
    {
        file_offset=nPatchNum*1024*1024+nSizeOnFtp;

    }
    else{
        file_offset=nPatchNum*1024*1024;
    }
    LOGI("thread[%d]:   file_offset==%d,waitToSendSize=%d,LINE==%d",m_nThread,file_offset,waitToSendSize,__LINE__);
    if((ret=lseek(pFile,file_offset,SEEK_SET))<0)
    {
        LOGI("thread[%d]:   lseek error...%s   errno=%d\n  %d",m_nThread,strerror(errno),errno,__LINE__);
    }
    //SendData(HeadFlag,);
    while(nRead<=waitToSendSize)
    {
            if((nRollRead=read(pFile,ReadBuf,4*1024))>0)
            {
                LOGI("thread[%d]:   read what??nRollRead= %d,RollNumber=%d,LINE==%d",m_nThread,nRollRead,RollNumber,__LINE__);
                if(SendData(ReadBuf,nRollRead)==-1)
                {
                    LOGI("thread[%d]:   上传文件包%s失败",m_nThread,pSendPath);
                    close(pFile);
                    return -1;
                }
                RollNumber++;
                memset(ReadBuf,0,4*1024);
                nRead+=nRollRead;
            }
            if(nRead==waitToSendSize)
            {
                LOGI("thread[%d]:   发送完成！！！！，LINE==%d",m_nThread,__LINE__);
                break;
            }
    }
    close(pFile);
    CloseDataSocket();
    if(RecvControl("226")==-1)
    {
        LOGI("thread[%d]:   close error...%s   errno=%d\n  %d",m_nThread,strerror(errno),errno,__LINE__);
        return -1;
    }
    LOGI("is there error???```````` no,LINE==%d",__LINE__);
    return 1;
}
void  CFTP::CloseDataSocket()
{
	if(m_DataSocket != INVALID_SOCKET)
	{
		close(m_DataSocket);
	}
	m_DataSocket = INVALID_SOCKET;
}
void  CFTP::CloseCtrlSocket()
{
	if(m_ControlSocket != INVALID_SOCKET)
	{
		SendAndRecv("QUIT \r\n","");
		close(m_ControlSocket);
	}
	m_ControlSocket = INVALID_SOCKET;

}

int socket_connect(char *host,char *port,int nThreadNumber)
{
    int sock,ret;
    struct sockaddr_in6 *ipv6;
    struct addrinfo hints,*res,*p;
    char ipv6str[128];

    //获得主机ipv6地址
    bzero(&hints,sizeof(hints));
    hints.ai_family=AF_INET6;
    hints.ai_socktype=SOCK_STREAM;
    //LOGI("thread[%d]:   获取地址,LINE==%d",nThreadNumber,__LINE__);
    if((ret=getaddrinfo(host,port,&hints,&res))!=0)
    {
        LOGI("thread[%d]:   get addrinfo error..info:%s",nThreadNumber,gai_strerror(ret));
        return -1;
    }
    for(p=res;p!=NULL;p=p->ai_next)
    {
       sock=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
       if(sock<0)
       {
            LOGI("thread[%d]:   socket create error...%s   errno=%d\n  %d",nThreadNumber,strerror(errno),errno,__LINE__);
            continue;
       }
       if(connect(sock,p->ai_addr,p->ai_addrlen)==0);
       {
            ipv6=(struct sockaddr_in6*)p->ai_addr;
            inet_ntop(p->ai_family,&ipv6->sin6_addr.s6_addr,ipv6str,sizeof(ipv6str));
            LOGI("thread[%d]:   %s-----> %s,%d",nThreadNumber,host,ipv6str,__LINE__);
            break;
       }
       close(sock);
    }
    if(p==NULL)
    {
        LOGI("TCP CONNECT ERROR");
        return -1;
    }
    freeaddrinfo(res);
    LOGI("thread[%d]:   %s,%d,LINE==%d",nThreadNumber,__FUNCTION__,sock,__LINE__);
    return sock;
}
/*struct timeval
  {
  __time_t tv_sec;        //Seconds   tv_sec为Epoch到创建struct timeval时的秒数
  __suseconds_t tv_usec;  // Microseconds. tv_usec为微秒数
  };
*/
/*
超时机制：等待30s
*/


int net_recv_timeout(int fd, char *buf, size_t len, uint32_t timeout,int thread)
{
    int ret;
    struct timeval tv;
    tv.tv_sec  = timeout / 1000;//30s
    tv.tv_usec = ( timeout % 1000 ) * 1000;//0
    memset(buf, 0, len);
    if( fd < 0 )
        return -1;

    fd_set read_fds;//哨岗监控集合
    FD_ZERO(&read_fds);//初始化文件集
    FD_SET(fd,&read_fds);////将需要监视的fd放入readfds集中
//设置监听，返回>0,超时返回0，发生错误，负数
    if((ret = select( fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv ))<=0)
    {
        LOGI(" thread[%d]   select error...%s   errno=%d\n  LINE==%d",thread,strerror(errno),errno,__LINE__);
        return -1;
    }
    if(FD_ISSET(fd,&read_fds))
    {
        ret = (int) recv( fd, (char *)buf, len, 0 );
        if( ret < 0 )
        {
            LOGI("thread[%d]sock connect recv error ...%s   errno=%d\n  %d",thread,strerror(errno),errno,__LINE__);
            return -1;
        }else{
            LOGI("thread[%d],%s,%s ,lINE==%d",thread,__FUNCTION__,rbuf,__LINE__);
        }
        return( ret );
    }
    else  {return -1;}
}
int  CFTP::GetSize(char * pszStr)
{
     char * pszPos =strstr(pszStr,"213 ");
     if(pszPos == NULL)
     {
         return -1;
     }
     pszPos += strlen("213 ");
     while (strstr(pszPos," ") == pszPos)
     {
         pszPos++;
     }
     return atoi(pszPos); //返回字节数
}
int CFTP::UpLoad(char *pszUpName,char *pszSave,int patchNum)
{
     //2018.01.03
     int ret;
     LOGI("thread[%d]:  20170104+++FTP %s,LINE==%d",m_nThread,__FUNCTION__,__LINE__);
     char szPathCmd[MAX_PATH +7]={0};
    //sprintf(szPathCmd,"SIZE %s\r\n",pszUpName);//FTP2018(N)
    // LOGI("thread[%d]:   20170104+++%s,LINE==%d",m_nThread,szPathCmd,__LINE__);
    /* if(SendAndRecv(szPathCmd,NULL)==-1)
     {
        return -1;//SIZE 命令发送失败
     }*/
     memset(m_szBuf,1,RECV_BUF_LEN);
     if(!strstr(m_szBuf,"213"))//说明文件不存在，直接上传
     {
         //3.上传文件
         LOGI("文件包直接上传，LINE==%d",__LINE__);
         sprintf(szPathCmd, "stor %s\r\n", pszSave);
         if((ret=SendAndRecv(szPathCmd,"150"))==-1)
         {
             LOGI("stor %s error,LINE==%d",pszSave,__LINE__);
             return -1;
         }
         //ReadAndSendFile(pszUpName,0);
         LOGI("thread[%d]:  is there error???--------NO!!!!,LINE==%d",m_nThread,__LINE__);
         if((ret=ReadAndSendPatch(pszUpName,patchNum,0))==1)
         {
            LOGI("发送包 %d 完成！！！LINE==%d",patchNum,__LINE__);
            return 1;
         }
         else {
            LOGI("thread[%d]:   上传 %d 文件包错误！！！，LINE==%d",patchNum,__LINE__);
         }
     }
     else{
         //说明文件本来就存在 断点续传
         int nSize=0;
         nSize=GetSize(m_szBuf);//获取文件大小
         LOGI("thread[%d]:  GetSize...%d",m_nThread,nSize);
         if (nSize <= 0 )
         {
             return 1;
         }
         sprintf(szPathCmd,"thread[%d]: APPE %s\r\n",m_nThread,pszUpName);
         if(!SendAndRecv(szPathCmd,"150"))
         {
            return -1;
         }
         if((ret=ReadAndSendPatch(pszUpName,patchNum,nSize))==1)
         {
            return 1;
         }
         else {
                      LOGI("thread[%d]:   上传 %d 文件包错误！！！，LINE==%d",patchNum,__LINE__);
         }
     }
}
int CFTP::CreateControlAndData()
{
    int ret=0,port;
    char ps[20];
    //2018.1.10
    char *u0 = "b1309cd87fcd0afe", *p0 = "JwogBE9r";
    char *u1 = "88b2437c660b3efd", *p1 = "l4frNgsz";
    char *u2 = "0d9796b0bc0d3983", *p2 = "8ZGNo2NC";
    char *u3 = "de7fc8e125d33820", *p3 = "rnIBe3U4";
    char *u,*p;
    char *c,*c2=NULL;
    if(m_nThread == 0)
    {u = u0; p = p0;}
    else if(m_nThread == 1)
    {u = u1; p = p1;}
    else if(m_nThread == 2)
    {u = u2; p = p2;}
    else if(m_nThread == 3)
    {u = u3; p = p3;}
    else return -1;
    LOGI("thread[%d]:   start upload_file",m_nThread);
//1.命令连接：客户端(N>1024)端口-->服务器21端口
    m_ControlSocket = socket_connect("ftp.openload.co", "21",m_nThread);
    /*if(ftp.CreateControlSocket("ftp.openload.co", "21",ftp.m_nTimeOut)==-1)
    {
        LOGI("20170104++命令连接 m_ControlSocket=%d，LINE==%d",ftp.m_ControlSocket,__LINE__);
        return -1;
    }*/
   LOGI("thread[%d]，%s ，control=%d，LINE==%d",m_nThread,__FUNCTION__,m_ControlSocket,__LINE__);
   if (m_ControlSocket< 0)
   {
        LOGI("thread[%d]:   error   LINE==%d",m_nThread,__LINE__);
        return -1;
   }
   ret = net_recv_timeout(m_ControlSocket, rbuf, sizeof(rbuf), 30*1000,m_nThread);
   if (ret == -1)
   {
        //LOGI("sock connect server error ...%s   errno=%d\n  %d",strerror(errno),errno,__LINE__);
        close(m_ControlSocket);
        return  -1;
   }
   else if(ret == 1){
        LOGI("thread[%d],%s,%s ,lINE==%d",m_nThread,__FUNCTION__,rbuf,__LINE__);
   }

//user
    sprintf(sbuf, "user %s\r\n", u);
    ret = send(m_ControlSocket, sbuf, strlen(sbuf), 0);
   // LOGI("thread[%d],%s",m_nThread,sbuf);
    ret = net_recv_timeout(m_ControlSocket, rbuf, sizeof(rbuf), 30 * 1000,m_nThread);
//pass
     sprintf(sbuf, "pass %s\r\n", p);
     ret = send(m_ControlSocket, sbuf, strlen(sbuf), 0);
     LOGI("thread[%d],%s",m_nThread,sbuf);
     ret = net_recv_timeout(m_ControlSocket, rbuf, sizeof(rbuf), 30 * 1000,m_nThread);
//type i---typebinary，设置二进制传输方式
    sprintf(sbuf, "type i\r\n");
    ret = send(m_ControlSocket, sbuf, strlen(sbuf), 0);
    LOGI("thread[%d],%s",m_nThread,sbuf);
    ret = net_recv_timeout(m_ControlSocket, rbuf, sizeof(rbuf), 30 * 1000,m_nThread);
//epsv 被动模式
    sprintf(sbuf, "EPSV ALL\r\n");
    ret = send(m_ControlSocket, sbuf, strlen(sbuf), 0);
    LOGI("thread[%d],%s",m_nThread,sbuf);
    if (ret == SOCKET_ERROR)
    {
        printf("EPSV send SOCKET_ERROR!\n");
        close(m_ControlSocket);
        return -1;
    }
    ret = net_recv_timeout(m_ControlSocket, rbuf, sizeof(rbuf), 30 * 1000,m_nThread);
    if (ret < 0)
    {
        printf("EPSV send net_recv_timeout SOCKET_ERROR!\n");
        close(m_ControlSocket);
        return -1;
    }
 //   LOGI("WHTA?????WHY");
    char *c1;
    int  b;
    c1 = strstr(rbuf, "|||");
    b= atoi(c1+3);
    LOGI("thread[%d]:   GET PORT %d",m_nThread,b);
//2.数据连接，客户端n+1端口-->ftp服务器ps端口
    char temp_port[20];
    sprintf(temp_port, "%d", b);
   // LOGI("thread[%d]:   数据连接,LINE==%d",m_nThread,__LINE__);
    m_DataSocket= socket_connect("ftp.openload.co", temp_port,m_nThread);
    LOGI("thread[%d]:   20170104  ++ %d   LINE==%d",m_nThread,m_DataSocket,__LINE__);
    if (m_DataSocket < 0)
    {
        close(m_ControlSocket);
        LOGI("thread[%d]:   error   LINE==%d",m_nThread,__LINE__);
        return -1;
    }
    return 1;
}


int CFTP::RecvAndWriteFile(char *pSavePath,int iFileSize)
{

    int bRes=-1;
    if(pSavePath!=NULL&&iFileSize>0)
    {
        char acRecv[32*512]={0};
        FILE * pFile=fopen(pSavePath,"wb");
        if(pFile!=NULL)
        {
            int iReturn =0;
            int bFindErr=-1;
            while(iFileSize>0)
            {
                int iNeedSize=iFileSize>TEMPSIZE?TEMPSIZE:iFileSize;
                iReturn=recv(m_ControlSocket,acRecv,iNeedSize,0);
                if(iReturn==SOCKET_ERROR)
                {
                    bFindErr=1;
                    break;
                }
                fwrite(acRecv,1,iReturn,pFile);
                iFileSize-=iReturn;
            }
            fclose(pFile);
            if(bFindErr==-1)
            {
                if(RecvControl("226")==1);
                {
                    bRes=1;
                }
            }

        }

    }
    CloseDataSocket();
    return bRes;
}

int CFTP::Download(char *pszFileName,char * pszSavePath)
{

    char szPathCmd[MAX_PATH+7]={0};
    sprintf(szPathCmd,"SIZE %s\r\n",pszFileName);
    if(SendAndRecv(szPathCmd,"213")==-1)
    {
        return -1;
    }
    int nSize=0;
    nSize=GetSize(m_szBuf);//获取文件大小
    if(nSize<0)
    {
        return -1;
    }
    sprintf(szPathCmd,"RETR %s\r\n",pszFileName);
    if(SendAndRecv(szPathCmd,"150")==-1)
    {
        return -1;
    }
    sprintf(szPathCmd,"%s\\%s",pszSavePath,pszFileName);
    if(!RecvAndWriteFile(szPathCmd,nSize))
    {
        return -1;
    }
    return 1;
}
int download_file(char *file)
{
    CFTP ftp;
    int index=1;
    LOGI("start upload_file、、、、%d", index);
    ftp.CreateControlAndData();
    ftp.Download(file_path,"/storage/emulated/0/temp/");
    return 0;
}
/*upload_id 0 :upload successful;
            1 :not upload;
            2 :uploading
*/
int UpdataProgress(const int total_count)
{
	pthread_mutex_lock(&mutex);
	int uploaded_count = 0;
	int tmp_i = 0;
	for (tmp_i = 0; tmp_i < total_count; tmp_i++) {
		if (patchStatusArray[tmp_i] == 0)
			uploaded_count++;
	}
	if(uploaded_count==total_count)
	is_upload_finish=1;
	pthread_mutex_unlock(&mutex);
	return 1;
}
int getPatch( char *patchName, char *prefix,char *suffix,int index){//包名，文件前缀 ，ftp2018，线程！！
    int i;
    memset(patchName,0,PATCH_FLAG_LEN);
    int patchNum = -1;//局部变量
    char patchNumString[4] = {0, 0, 0, 0};
    pthread_mutex_lock(&mutex);
    for(i=0;i<patchsCount;i++)
    {
        LOGI("thread[%d]:   patchStatusArray[%d]==%d,LINE==%d",index,i,patchStatusArray[i],__LINE__);
        if(patchStatusArray[i]==2)//uploading
        {
            patchNum=-2;
        }
        if(patchStatusArray[i]==1)
        {
            patchNum=i;
            LOGI("thread[%d]: patchNum==%d,LINE==%d",index,patchNum,__LINE__);
            patchStatusArray[i]=2;
            break;
        }
    }
    //LOGI("patchNum==%d,LINE==%d",patchNum,__LINE__);
    if(patchNum>=0)
    {
        strcat( patchName, prefix );
        sprintf( patchNumString, "%d", patchNum );
        strcat(patchName,"(");
        strcat( patchName, patchNumString );    //FTP2018(1??)
        strcat(patchName,")");
        strcat(patchName,suffix);
        LOGI("thread[%d]:   patchName==%s",index,patchName);
    }
    pthread_mutex_unlock(&mutex);
    return patchNum;
}
int UpdataProfile(char *temp_path,int index)
{
    LOGI("开始配置？？？，LINE==%d",__LINE__);
    int profile_fp;
    int ret;
    if((profile_fp=open(temp_path,O_RDWR))==-1)
    {
         LOGI("thread[%d]:  open %s error : %s ,LINE==%d",index,temp_path,strerror(errno),__LINE__);
         return -1;
    }
    if((ret=write(profile_fp,patchStatusArray,sizeof(patchStatusArray)))==-1)
    {
        LOGI("thread[%d]:   patchStatus: %d,%d,%d,LINE==%d",index,patchStatusArray[0],patchStatusArray[1],patchStatusArray[2],patchStatusArray[3],__LINE__);
        LOGI("thread[%d]:   write %s error : %s ,LINE==%d",index,temp_path,strerror(errno),__LINE__);
    }
    LOGI("Updata Success!!!!,LINE==%d",__LINE__);
    close(profile_fp);
    return 1;
}
int initProfile(char *temp_path)
{
     int profile_fp;//定义一个文件指针;
     int ret;
     char str[5000];
     LOGI("profile is not exit,create it now!!  LINE==%d",__LINE__);
     profile_fp=open(temp_path,O_CREAT| O_RDWR);//创建可读、写文件
     if(profile_fp==-1)
     {
        LOGI("open %s error : %s ,LINE==%d",temp_path,strerror(errno),__LINE__);
     }
     if((ret=write(profile_fp,patchStatusArray,sizeof(patchStatusArray)))==-1)
     {
        LOGI("patchStatus: %d,%d,%d,LINE==%d",patchStatusArray[0],patchStatusArray[1],patchStatusArray[2],patchStatusArray[3],__LINE__);
        LOGI("write %s error : %s ,LINE==%d",temp_path,strerror(errno),__LINE__);
     }
     close(profile_fp);
     LOGI("配置成功！！！----2,LINE==%d",__LINE__);
     return 1;
}
int initPatchStatusArray(int temp_count)
{
    memset(patchStatusArray,0,MAX_PATCHS*sizeof(char));
    int temp_i;
    for(temp_i=0;temp_i<temp_count;temp_i++)
    patchStatusArray[temp_i]=1; //not upload;
    return 1;
}
int UpdataPatchStatusArray(int patchNo,char uploadStatus)
{
//加锁
    pthread_mutex_lock(&mutex);
    patchStatusArray[patchNo]=uploadStatus;
    pthread_mutex_unlock(&mutex);
//解锁
    return 1;
}

void* upload_file_thread(void *arg){
    CFTP ftp;
    int index=*(int*)arg;
    ftp.m_nThread=index;
    LOGI("thread -----%d  :  start upload_file!!!!", index);
    char patchName[50];
    int patchNum;
    int count=8;
    int ret;
    LOGI("is_upload_finish==%d,LINE==%d",is_upload_finish,__LINE__);

    while(is_upload_finish==0)
    {
        if(ftp.CreateControlAndData()==1)//连接成功
        {
            LOGI("thread[%d]:   %s,获取patchnumber ,LINE==%d",index,__FUNCTION__,__LINE__);
            patchNum=getPatch(patchName,patchFlag,JapanSuffix,index);
            if(patchNum==-1)//all upload succeed;
            {
                is_upload_finish=1;
                break;
            }
            else if(patchNum==-2)//there is another process upload,wait
            {
                sleep(5);
                continue;
            }
           if((ret=ftp.UpLoad(patchName,patchName,patchNum))==-1)//传完一个包，更新配置文件！！
           {
                LOGI("thread[%d]:  ftp Upload error 返回值----%d ,LINE==%d",index,ret,__LINE__);
                patchStatusArray[patchNum]=1;
           }
           else if(ret==1) {
                LOGI("发包成功！！！并结束！！！,LINE==%d",__LINE__);
                if((ret=UpdataPatchStatusArray(patchNum,0))==1)
                {
                    LOGI("更新成功，即将写入配置文件中！！！");
                    UpdataProfile(PROFILE,index);
                    UpdataProgress(patchsCount);
                }

           }
        }
    }
    if(is_upload_finish==1)
    {
        pthread_exit(0);
    }
}
int ftpupload_file(char *pSendPath,char *profile)
{
    //每个块1m。
    int nFileSize = GetLocalFileSize(pSendPath);
    patchSize=512*1024;//  分片大小512kb；
    rollPerSocket=64;
    patchsCount=nFileSize/patchSize;
    is_upload_finish=0;
    long someExtra=nFileSize%patchSize;
    if(someExtra>0)
    {
        patchsCount++;
    }
    LOGI("patchsCount==%d,LINE==%d",patchsCount,__LINE__);
    if(access(profile,F_OK)!=0)
    {
        initPatchStatusArray(patchsCount);
        initProfile(profile);
       // LOGI("IS there??????    NO!!!! LINE==%d",__LINE__);
    }
    else   if(access(profile,F_OK)==0)
    {
         int profile_fp;
         int ret;
         memset(patchStatusArray,0,MAX_PATH*sizeof(char));
         profile_fp=open(profile,O_RDWR);
         if((ret=read(profile_fp,patchStatusArray,sizeof(patchStatusArray)))<0)
         {
                LOGI("read %s error : %s ,LINE==%d",profile,strerror(errno),__LINE__);
         }
         for(int temp_i=0;temp_i<patchsCount;temp_i++)
         {
                if(patchStatusArray[temp_i]==2)
                    patchStatusArray[temp_i]=1;
         }
         close(profile_fp);
         LOGI("test patchStatus: %d,%d,%d,LINE==%d",patchStatusArray[0],patchStatusArray[1],patchStatusArray[2],patchStatusArray[3],__LINE__);
    }
    int res= pthread_mutex_init(&mutex,NULL);
    if(0!=res)
    {
        LOGI("mutex init error");
    }
  // patchNum=getPatchUploadingNum(PROFILE);
    int ii[4]={0,1,2,3};
    for(int i=0;i<4;i++)
    {
    //参数1：指向线程标识符的指针;参数2：设置线程属性;参数3：线程运行函数的起始地址；参数4：运行函数的参数
        int err=pthread_create(&tid[i],NULL,upload_file_thread,&ii[i]);
        if(err!=0) LOGI("can't create thread:%s",strerror(err));
    }
    for(int j=0;j<4;j++)
    {
        pthread_join(tid[j],NULL);
    }
    return 0;
}
int one_ftpupload_file(char *pSendPath,char *profile)
{

}


JNIEXPORT jstring JNICALL Java_com_example_myapplication_MainActivity_FtpDownloadFromJNI(JNIEnv *, jobject)
{
    download_file(file_path);
}
JNIEXPORT jstring JNICALL Java_com_example_myapplication_MainActivity_FtpUploadFromJNI(JNIEnv *, jobject)
{
    LOGI("2018/1/17/SHANGCHUAN!!!!!!!!heiheihiehei  66666666~~~");
    ftpupload_file(file_path,PROFILE);
}
JNIEXPORT jstring JNICALL Java_com_example_myapplication_MainActivity_OneFtpUploadFromJNI(JNIEnv *, jobject)
{
    LOGI("2018/1/17/SHANGCHUAN!!!!!!!!heiheihiehei  66666666~~~");
    one_ftpupload_file(file_path,PROFILE);
}

