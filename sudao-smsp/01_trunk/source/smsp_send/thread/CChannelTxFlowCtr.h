#ifndef __H_CHANNEL_TX_FLOW_CTR_
#define __H_CHANNEL_TX_FLOW_CTR_

#include "platform.h"
#include "Comm.h"

/* 流控状态*/
enum
{
	CHANNEL_FLOW_STATE_INIT,
	CHANNEL_FLOW_STATE_NORMAL,
	CHANNEL_FLOW_STATE_BLOCK,
	CHANNEL_FLOW_STATE_CLOSE,
	CHANNEL_FLOW_STATE_MAX,
};

enum
{
	FLOW_CTR_CALLER_NONE,
	FLOW_CTR_CALLER_MQ,
	FLOW_CTR_CALLER_POOL,
};

typedef struct CHANNELSHAREINFO
{
	CHANNELSHAREINFO()
	{
		uChannelState = 0;
		uSliderWindowNum = 0;
		bSwitchOff = false;
		bClusterSwi = false;
	}
	UInt32 uChannelState;	 //通道状态
	Int32  uSliderWindowNum; //滑窗个数
	Int32  bSwitchOff;       //临时关闭
	bool   bClusterSwi;     //组包逻辑滑窗
}channelShareInfo;

class CChanelTxFlowCtr
{
public:
	static Int32  channelFlowCtrCountAdd( UInt32 uChannelId, Int32 uMaxWindowSize, Int32 iCount = 1  );
	static Int32  channelFlowCtrCountSub( Int32 caller, UInt32 uChannelId, UInt32 uMaxWindowSize = 0, Int32 iCount = 1 );
	static UInt32 channelFlowCtrCheckState( UInt32 uChannelId );
	static bool   channelFlowCtrSetState( UInt32 uChannelId, UInt32 uState );
	static bool   channelFlowCtrSetSize( UInt32 uChannelId, UInt32 uSize, bool bCluster = false );
	static bool   channelFlowCtrDelete( UInt32 uChannelId );
	static bool   channelFlowCtrNew( UInt32 uChannelId, UInt32 uWindowSize, bool bSwitchOff = false , bool bCluster = false );
	static bool   channelFlowInit();
	static int    channelFlowCtrGetSize(UInt32 uChannelId);
	static bool   channelFlowCtrSynchronous(UInt32 uChannelId, int iWndSize);
};

extern map<UInt32,channelShareInfo> g_mapChannelShareInfo;
extern pthread_mutex_t g_channelShareMutex;

#endif
