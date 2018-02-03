#include<vector>
using std::vector;
#define RECV_BUF_LEN 1024 //接收命令字符数组长度
#define PATCH_FLAG_LEN 50
#define PATCH_SEND_LEN 1*1024*1024 //文件包1M

#define MAX_PATH 260
#define uint32_t int
#define THREADNUMBER 5
#define INVALID_SOCKET  0xFFFFFFFF
#define SOCKET_ERROR    (-1)
#define SO_DONTLINGER   (int)(~SO_LINGER)
#define MAX_PATCHS 5000 //最多5G;
#define SENDBUFFER 8*512 // 每次传4kb
#define TEMPSIZE (32*512)



char patchFlag[PATCH_FLAG_LEN]="FTP2018";
unsigned char patchStatusArray[MAX_PATCHS];//多线程，共享内存，记录patch状态。
char patchSuffix[PATCH_FLAG_LEN]="我的文件包.rar";
char JapanSuffix[PATCH_FLAG_LEN]="私のパッケージ.rar";


int patchSize=0;
int patchsCount=0;
int rollPerSocket=0;
int is_upload_finish;

pthread_t tid[4];
pthread_mutex_t mutex;//定义互斥锁;

class CFTP
{
    public:
        CFTP(void);
        ~CFTP(void);
        int m_ControlSocket;
        int m_DataSocket;
        int m_nTimeOut;////套接字多久没反应 自动返回
        int m_nThread;//线程编号！！！
    protected:
        char m_szBuf[RECV_BUF_LEN];//接收命令回复数组
    protected:
        int SendControl(const char * pszData);//给服务器发送命令
        int RecvControl(const char * pszResult);//接收命令回复

    public:
        int CreateControlAndData();
        int  SendAndRecv(const char * pszCMD,char * pszResult);//发送命令并判断是否成功执行
        //int  SendAndRecv(const char * pszCMD,char * pszResult);
        int  RecvData(char * pszBuf,int iLength);//接收数据
        int  SendData(char *pszData,int nLength);//发送的数据长度
        int  ReadAndSendPatch(char *pSendPath,int nPatchNum,int nSizeOnFtp);
        int  ReadAndSendFile(char *pSendth,int nFtpFileSize);
      //  int  UpLoad(char *pszUpName,char * pszSave);
        int UpLoad(char *pszUpName,char *pszSave,int patchNum);
        int RecvAndWriteFile(char *pSavePath,int iFileSize);//下载文件路径，文件大小
        int Download(char *pszUpName,char *pSavePath);
        int  GetSize(char * pszStr);//获取ftp文件大小

        void CloseDataSocket();
        void CloseCtrlSocket();

    public:
        int IsRecvAlready;
        static int DATA_TRY_TIMES;//重试次数
        static int DATA_TRY_INTERVAL;//重试间隔
};



