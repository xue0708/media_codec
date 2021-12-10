/**************************************************************************************
File��HKCapture.cpp
Description������SDK�ɼ�RTSP������HKCapture�г�Ա������ʵ��
**************************************************************************************/
#include "HKCapture.h"
#include <pthread.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

void *GetStream(void*);
long g_palyhandle;
//ȫ�ֲ��ſ�port��
LONG lPort[20];     
//�߳���               
pthread_mutex_t mutex[20]; 
//ͼ������          
vector<queue<Mat> >g_frameLists;   
int num =0;

/**************************************************************************************
Function��DecCBFun
Description���ص���������ʽת����YUV to BGR
**************************************************************************************/
void CALLBACK DecCBFun(int	nPort, char* pBuf, int nSize, FRAME_INFO* pFrameInfo, void* nUser, int nReserved2)
{
	DWORD nReserved = *(DWORD*)nUser;
	int lFrameType = pFrameInfo->nType; 
	if(lFrameType ==T_YV12)
	{
		Mat g_BGRImage;
		g_BGRImage.create(pFrameInfo->nHeight,pFrameInfo->nWidth,CV_8UC3);
		Mat YUVImage(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);
		cvtColor(YUVImage, g_BGRImage, COLOR_YUV2BGR_YV12);
		
		pthread_mutex_lock(&mutex[nReserved]);
		g_frameLists[nReserved].push(g_BGRImage);
		pthread_mutex_unlock(&mutex[nReserved]);
		YUVImage.~Mat();
	}
}

/**************************************************************************************
Function��ExceptionCallBack
Description���ص����������쳣�׳���������
**************************************************************************************/
void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = {0};
	switch(dwType) 
	{
	case EXCEPTION_RECONNECT:    
		break;
	default:
		break;
	}
}

/**************************************************************************************
Function��fRealDataCallBack
Description��ʵʱ���ص����������ϻ�ȡ������
**************************************************************************************/
void CALLBACK fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, DWORD pUser)
{
	DWORD dRect,index;
	index = lRealHandle;
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD:    
		if(!PlayM4_GetPort(&lPort[index])) 
		{
			dRect = PlayM4_GetLastError(lPort[index]);
			break;
		}
		if(dwBufSize >0)
		{
			//�����ӿ�
			if (!PlayM4_OpenStream(lPort[index], pBuffer, dwBufSize, 1024*1024)) 
			{
				dRect = PlayM4_GetLastError(lPort[index]);
				break;
			}
			//���ý��뺯��
			DWORD *tmp=new DWORD();
			*tmp=pUser;
			if(!PlayM4_SetDecCallBackMend(lPort[index], DecCBFun,tmp))
			{
				dRect = PlayM4_GetLastError(lPort[index]);
				break;
			}
			if (!PlayM4_Play(lPort[index], 0)) 
			{
				dRect = PlayM4_GetLastError(lPort[index]);
				break;
			}
		}
		break;
	case NET_DVR_STREAMDATA:   
		if (dwBufSize > 0 && lPort[index] != -1)
		{
			if (!PlayM4_InputData(lPort[index], pBuffer, dwBufSize))
			{
				dRect = PlayM4_GetLastError(lPort[index]);
				break;
			} 
		}
		break;  
	}       
}

HKCapture::HKCapture()
{
	queue<Mat> frameQueue;
	g_frameLists.push_back(frameQueue);
	cam_id = num;
	num++;
}

HKCapture::~HKCapture()
{
    ReleaseCamera();
}

/**************************************************************************************
Function��ReleaseCamera
Description���ͷ��������Դ
**************************************************************************************/
int HKCapture::ReleaseCamera(void)
{
    if(!NET_DVR_StopRealPlay(lRealPlayHandle))
	{
        printf("NET_DVR_StopRealPlay error! Error number: %d\n",NET_DVR_GetLastError());
        return 0;
    }
    NET_DVR_Logout(lUserID);
    NET_DVR_Cleanup();
    return 1;
}

/**************************************************************************************
Function��InitHKNetSDK
Description������SDK��ʼ��
**************************************************************************************/
void HKCapture::InitHKNetSDK()
{
    NET_DVR_Init();
    NET_DVR_SetConnectTime(200, 1);
    NET_DVR_SetReconnect(10000, true);
}

/**************************************************************************************
Function��InitCamera
Description����������ɼ�ʱ��Ҫ�Ĳ���
**************************************************************************************/
CamHandle HKCapture::InitCamera(const char *sIP, const char *UsrName, const char *PsW, const int Port, const int streamType, string winname)
{
	m_name = winname;
	NET_DVR_DEVICEINFO_V40 struDeviceInfo = {0};
	NET_DVR_USER_LOGIN_INFO LoginInfo = {0};
	strcpy(LoginInfo.sDeviceAddress,sIP);
	strcpy(LoginInfo.sUserName ,UsrName);
	strcpy(LoginInfo.sPassword,PsW);
	LoginInfo.wPort = Port;

    lUserID = NET_DVR_Login_V40(&LoginInfo, &struDeviceInfo);
    if (lUserID < 0)
	{
        printf("Login error, %d\n", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return -1;
    }
    NET_DVR_SetExceptionCallBack_V30(0, NULL,ExceptionCallBack, NULL);
	//��ȡ������
	NET_DVR_PREVIEWINFO struPlayInfo = {0};
	struPlayInfo.hPlayWnd     = 0;
	struPlayInfo.lChannel     = 1;  
	struPlayInfo.dwLinkMode   = 0;
	struPlayInfo.bBlocked = 1;
	struPlayInfo.dwDisplayBufNum = 1;
	//������������
	struPlayInfo.dwStreamType = streamType;    
	lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, NULL, NULL);
	g_palyhandle = lRealPlayHandle;
    if (lRealPlayHandle<0)
	{
		printf("pyd--NET_DVR_RealPlay_V40  error, %d\n", NET_DVR_GetLastError());
		return -1;
	}
	int iRet = NET_DVR_SetRealDataCallBack(lRealPlayHandle, fRealDataCallBack, cam_id);
	if (!iRet)
	{
		printf("pyd---SetRealDataCallBack error\n");
		NET_DVR_StopRealPlay(lRealPlayHandle); 
	}
	
    return lRealPlayHandle;
}

/**************************************************************************************
Function��GetCamMat
Description��������ȡ��������һ֡ͼ��
**************************************************************************************/
int HKCapture::GetCamMat(Mat &Img)
{
	pthread_mutex_lock(&mutex[cam_id]);
	if(!g_frameLists[cam_id].empty())
	{
		Img = g_frameLists[cam_id].front();
		g_frameLists[cam_id].pop();
	}
	pthread_mutex_unlock(&mutex[cam_id]);

	return -1;
}
