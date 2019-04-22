#include "CChannelTxFlowCtr.h"
#include "CLogThread.h"

map<UInt32,channelShareInfo> g_mapChannelShareInfo;
pthread_mutex_t g_channelShareMutex;

/*�������ؼ���,  �������Ӻ������*/
Int32 CChanelTxFlowCtr::channelFlowCtrCountAdd( UInt32 uChannelId, Int32 uMaxWindowSize, Int32 iCount )
{
	Int32 retVal = 0;
	pthread_mutex_lock(&g_channelShareMutex);

	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{	
		if ( itrShare->second.bSwitchOff ){
			retVal = uMaxWindowSize;
		}
		else
		{
			itrShare->second.uSliderWindowNum += iCount;
			
			if(uMaxWindowSize >= 0 && itrShare->second.uSliderWindowNum > uMaxWindowSize )
			{
				itrShare->second.uSliderWindowNum = uMaxWindowSize;
			}
			
			retVal = itrShare->second.uSliderWindowNum;
		}
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	return retVal;

}

/* �������ؼ���, ���ؼ��ٺ������*/
Int32 CChanelTxFlowCtr::channelFlowCtrCountSub( Int32 caller, UInt32 uChannelId, UInt32 uMaxWindowSize, Int32 iCount )
{
	Int32 retVal = 1;//����״̬���滺����У��޻������
	pthread_mutex_lock(&g_channelShareMutex);
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		if ( !itrShare->second.bSwitchOff &&  
			!( itrShare->second.bClusterSwi && caller == FLOW_CTR_CALLER_MQ ))
		{
			itrShare->second.uSliderWindowNum -= iCount;
			retVal = itrShare->second.uSliderWindowNum;
		}else{
			retVal = itrShare->second.uSliderWindowNum;
		}
	}
	
	pthread_mutex_unlock(&g_channelShareMutex);
	
	return retVal;

}


/* ��������״̬*/
bool CChanelTxFlowCtr::channelFlowCtrSetState( UInt32 uChannelId, UInt32 uState )
{
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		itrShare->second.uChannelState = uState ;
	}
	pthread_mutex_unlock(&g_channelShareMutex);

	return true;
}


/*��ȡ����״̬*/
UInt32 CChanelTxFlowCtr::channelFlowCtrCheckState( UInt32 uChannelId )
{
	/* Ĭ�Ϸ�������������״̬���滺����� */
	UInt32 retState = CHANNEL_FLOW_STATE_NORMAL;
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		if( itrShare->second.uSliderWindowNum >= 0 
			|| itrShare->second.bSwitchOff )
		{
			retState = CHANNEL_FLOW_STATE_NORMAL;
		}
		else
		{
			retState = CHANNEL_FLOW_STATE_BLOCK;
		}
	}

	pthread_mutex_unlock(&g_channelShareMutex);

	return retState;
}

bool CChanelTxFlowCtr::channelFlowCtrSynchronous(UInt32 uChannelId, int iWndSize)
{	
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		itrShare->second.uSliderWindowNum = iWndSize ;
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	LogNotice("ChannelId[%u] CurrWnd[%d]", uChannelId, itrShare->second.uSliderWindowNum);
	return true;
}

int CChanelTxFlowCtr::channelFlowCtrGetSize(UInt32 uChannelId)
{
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		pthread_mutex_unlock(&g_channelShareMutex);
		return itrShare->second.uSliderWindowNum;
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	return 0;
}

/*����ͨ������*/
bool CChanelTxFlowCtr::channelFlowCtrSetSize( UInt32 uChannelId, UInt32 uSize, bool bCluster )
{
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		itrShare->second.bSwitchOff = ( uSize == 0 ) ? true : false ;
		itrShare->second.uSliderWindowNum = uSize ;
		itrShare->second.bClusterSwi = bCluster;
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	return true;
}

/*ɾ��ͨ�����ٿ��ƽṹ*/
bool CChanelTxFlowCtr::channelFlowCtrDelete( UInt32 uChannelId )
{
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare != g_mapChannelShareInfo.end())
	{
		g_mapChannelShareInfo.erase(itrShare);
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	return true;
}


/*ͨ�����뵽����*/
bool CChanelTxFlowCtr::channelFlowCtrNew( UInt32 uChannelId, UInt32 uWindowSize, bool bSwitchOff, bool bCluster )
{
	pthread_mutex_lock( &g_channelShareMutex );
	map<UInt32,channelShareInfo>::iterator itrShare = g_mapChannelShareInfo.find( uChannelId );
	if ( itrShare == g_mapChannelShareInfo.end())
	{
		channelShareInfo info;
		info.uChannelState =  CHANNEL_FLOW_STATE_NORMAL;
		info.uSliderWindowNum = ( uWindowSize == 0  ?  1 : uWindowSize );
		info.bSwitchOff = bSwitchOff;
		info.bClusterSwi = bCluster;
		g_mapChannelShareInfo[ uChannelId ] = info;
	}
	pthread_mutex_unlock(&g_channelShareMutex);
	return true;
}

/*ͨ�����س�ʼ��*/
bool CChanelTxFlowCtr::channelFlowInit()
{
	g_mapChannelShareInfo.clear();
	pthread_mutex_init(&g_channelShareMutex, NULL );
	return true;
}
