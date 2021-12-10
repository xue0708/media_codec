/**************************************************************************************
File��HKCapture.h
Description������SDK�ɼ�RTSP����ͷ�ļ���������HKCapture
**************************************************************************************/
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "HCNetSDK.h"
#include "PlayM4.h"
#include "LinuxPlayM4.h"
#include "opencv2/opencv.hpp"

using namespace cv;  
using namespace std;
                                                    
typedef long CamHandle;                                       

/**************************************************************************************
Function����HKCapture
Description��InitHKNetSDK()��SDK�ĳ�ʼ����
             InitCamera()������ɼ�ʱ����Ĳ�����
			 ReleaseCamera���ͷ��������Դ��
			 GetCamMat()����ȡ������һ֡ͼ��
**************************************************************************************/
class HKCapture
{
public:
    HKCapture();
    ~HKCapture();

    void InitHKNetSDK();
    CamHandle InitCamera(const char *sIP, const char *UsrName,const char *PsW,const int Port,const int streamType, string winname);
    int ReleaseCamera(void);
	int GetCamMat(Mat &Img);
	
	string m_UserName,m_sIP,m_Psw, m_strStreamType;
	DWORD  m_streamType;
	LONG lRealPlayHandle;
    LONG lUserID;
	string m_name;
	DWORD cam_id;
};

