
#ifndef _COMMON_H_
#define _COMMON_H_

#include "reactor.h"

#ifdef _WIN32
	#define close(handle) closesocket(handle)
	#define strncasecmp   _strnicmp
	#pragma warning(disable: 4996)
#endif

/// 检查句柄是否有效
extern bool IsValidHandle(reactor::handle_t handle)
{
#if defined(_WIN32)
    return handle != INVALID_SOCKET;
#elif defined(__linux__)
    return handle >= 0;
#else
    return false;
#endif
}

/// 反馈socket error
extern void ReportSocketError(const char * msg)
{
#if defined(_WIN32)
    fprintf(stderr, "%s error: %d\n", msg, WSAGetLastError());
#elif defined(__linux__)
    fprintf(stderr, "%s error: %s\n", msg, strerror(errno));
#endif
}
#endif // _COMMON_H_