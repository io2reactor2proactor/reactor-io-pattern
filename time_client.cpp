#ifndef _TIME_CLIENT_H_
#define _TIME_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#ifdef __linux__
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif //__linux__

#include "common.h"

#endif // _TIME_CLIENT_H_

/// @file   reactor_client_test.cc
/// @brief  ÓÃreactorÊµÏÖµÄ¿Í»§¶Ë,ÓÃtelnetÐ­Òé
/// @author zeshengwu<wuzesheng@vip.qq.com>
/// @date   2011-03-30

/// 全局反应器对象
reactor::Reactor g_reactor;

/// 定义读写缓冲区
const size_t kBufferSize = 1024;
char g_read_buffer[kBufferSize];
char g_write_buffer[kBufferSize];

class TimeClient : public reactor::EventHandler
{
public:

    /// 构造函数
    TimeClient() : EventHandler()
    {
        m_handle = socket(AF_INET, SOCK_STREAM, 0);
        assert(IsValidHandle(m_handle));
    }

    /// 析构函数
    ~TimeClient()
    {
        close(m_handle);
    }

    /// 创建socket
    bool ConnectServer(const char * ip, unsigned short port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);
        if (connect(m_handle, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            ReportSocketError("connect");
            return false;
        }
        return true;
    }

    /// 获取文件描述符句柄
    virtual reactor::handle_t GetHandle() const
    {
        return m_handle;
    }

    /// 读数据
    virtual void HandleRead()
    {
        memset(g_read_buffer, 0, kBufferSize);
        int len = recv(m_handle, g_read_buffer, kBufferSize, 0);
        if (len > 0)
        {
            fprintf(stderr, "%s", g_read_buffer);
            g_reactor.RegisterHandler(this, reactor::kWriteEvent);
        }
        else
        {
            ReportSocketError("recv");
        }
    }

    /// 写数据
    virtual void HandleWrite()
    {
        memset(g_write_buffer, 0, kBufferSize);
        int len = sprintf(g_write_buffer, "time\r\n");
        len = send(m_handle, g_write_buffer, len, 0);
        if (len > 0)
        {
            fprintf(stderr, "%s", g_write_buffer);
            g_reactor.RegisterHandler(this, reactor::kReadEvent);
        }
        else
        {
            ReportSocketError("send");
        }
    }

    /// scoket error处理
    virtual void HandleError()
    {
        fprintf(stderr, "server closed\n");
        close(m_handle);
        exit(0);
    }

private:

    reactor::handle_t  m_handle;  ///< 文件描述符句柄
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
        return EXIT_FAILURE;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        fprintf(stderr, "WSAStartup() error:%d\n", WSAGetLastError());
        assert(0);
    }
#endif

    TimeClient client;
    if (!client.ConnectServer(argv[1], atoi(argv[2])))
    {
        fprintf(stderr, "connect remote server failed\n");
        return EXIT_FAILURE;
    }

    g_reactor.RegisterHandler(&client, reactor::kWriteEvent);
    while (1)
    {
        g_reactor.HandleEvents(100);
#if defined(_WIN32)
        Sleep(1000);
#elif defined(__linux__)
        sleep(1);
#endif
    }
    g_reactor.RemoveHandler(&client);
#ifdef _WIN32
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}