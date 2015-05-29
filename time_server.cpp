#ifndef _TIME_SERVER_H_
#define _TIME_SERVER_H_

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

#include <string>
#include "common.h"

#endif // _TIME_SERVER_H_

/// @file   reactor_server_test.cpp
/// @brief
/// @author lovezhangkai@foxmail
/// @date   2013-10-1

/// 全局反应器对象
reactor::Reactor g_reactor;

/// 定义读写缓冲区
const size_t kBufferSize = 1024;
char g_read_buffer[kBufferSize];
char g_write_buffer[kBufferSize];

class RequestHandler : public reactor::EventHandler
{
public:

    /// 构造函数
    RequestHandler(reactor::handle_t handle) : EventHandler(), m_handle(handle) {}

    /// 获取文件描述符句柄
    virtual reactor::handle_t GetHandle() const
    {
        return m_handle;
    }

    /// 写数据
    virtual void HandleWrite()
    {
        memset(g_write_buffer, 0, sizeof(g_write_buffer));
        int len = sprintf(g_write_buffer, "current time: %d\r\n", (int)time(NULL));
        len = send(m_handle, g_write_buffer, len, 0);
        if (len > 0)
        {
            fprintf(stderr, "send response to client, fd=%d\n", (int)m_handle);
            g_reactor.RegisterHandler(this, reactor::kReadEvent);
        }
        else
        {
            ReportSocketError("send");
        }
    }

    /// 读数据
    virtual void HandleRead()
    {
        memset(g_read_buffer, 0, sizeof(g_read_buffer));
        int len = recv(m_handle, g_read_buffer, kBufferSize, 0);
        if (len > 0)
        {
            if (strncasecmp("time", g_read_buffer, 4) == 0)
            {
                g_reactor.RegisterHandler(this, reactor::kWriteEvent);
            }
            else if (strncasecmp("exit", g_read_buffer, 4) == 0)
            {
                close(m_handle);
                g_reactor.RemoveHandler(this);
                delete this;
            }
            else
            {
                fprintf(stderr, "Invalid request: %s", g_read_buffer);
                close(m_handle);
                g_reactor.RemoveHandler(this);
                delete this;
            }
        }
        else
        {
            ReportSocketError("recv");
        }
    }

    virtual void HandleError()
    {
        fprintf(stderr, "client %d closed\n", m_handle);
        close(m_handle);
        g_reactor.RemoveHandler(this);
        delete this;
    }

private:

    reactor::handle_t m_handle; ///< 文件描述符句柄
};

class TimeServer : public reactor::EventHandler
{
public:

    /// 构造函数
    TimeServer(const char * ip, unsigned short port) : EventHandler(), m_ip(ip), m_port(port) {}

    /// 创建socket
    bool Start()
    {
        m_handle = socket(AF_INET, SOCK_STREAM, 0);
        if (!IsValidHandle(m_handle))
        {
            ReportSocketError("socket");
            return false;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
        if (bind(m_handle, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            ReportSocketError("bind");
            return false;
        }

        if (listen(m_handle, 10) < 0)
        {
            ReportSocketError("listen");
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
        struct sockaddr addr;
#if defined(_WIN32)
        int addrlen = sizeof(addr);
#elif defined(__linux__)
        socklen_t addrlen = sizeof(addr);
#endif
        reactor::handle_t handle = accept(m_handle, &addr, &addrlen);
        if (!IsValidHandle(handle))
        {
            ReportSocketError("accept");
        }
        else
        {
            RequestHandler * handler = new RequestHandler(handle);
            if (g_reactor.RegisterHandler(handler, reactor::kReadEvent) != 0)
            {
                fprintf(stderr, "error: register handler failed\n");
                delete handler;
            }
        }
    }

private:

    reactor::handle_t     m_handle; ///< 文件描述符句柄
    std::string           m_ip;     ///< IP地址
    unsigned short        m_port;   ///< 端口号
};

int main(int argc, char ** argv)
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
        fprintf(stderr, "WSAStartup() error: %d\n", WSAGetLastError());
        return false;
    }
#endif

    TimeServer server(argv[1], atoi(argv[2]));
    if (!server.Start())
    {
        fprintf(stderr, "start server failed\n");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "server started!\n");

    while (1)
    {
        g_reactor.RegisterHandler(&server, reactor::kReadEvent);
        g_reactor.HandleEvents(100);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}