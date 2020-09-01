//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
namespace knet {

namespace kcp { 
enum KcpMsgType { KCP_SHAKEHAND_REQUEST = 250, KCP_SHAKEHAND_RESPONSE, KCP_HEARTBEAT, KCP_GOODBYE };

struct KcpMsgHead {
	uint32_t conv;
	uint8_t cmd;
	uint8_t frg;
	uint16_t wnd;
};

struct KcpShakeHandMsg {
	uint32_t conv = 0;
	uint8_t cmd = 0;
	uint8_t frg = 0;
	uint16_t wnd = 0;
};
using KcpHeartbeat = KcpShakeHandMsg;

} // namespace  kcp

} // namespace knet
