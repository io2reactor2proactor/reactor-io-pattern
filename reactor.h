#ifndef _REACTOR_H_
#define _REACTOR_H_

#ifdef _WIN32
	#define FD_SETSIZE 8192
	#include <Winsock2.h>
#elif defined(__linux__)
	#include <stdint.h>
	#include <unistd.h>
	#include <sys/epoll.h>
#endif

/// @file   reactor.h
/// @brief
/// @author lovezhangkai@foxmail
/// @date   2013-10-1

namespace reactor
{

typedef unsigned int event_t;
enum
{
    kReadEvent    = 0x01, ///<读事件掩码
    kWriteEvent   = 0x02, ///<写事件掩码
    kErrorEvent   = 0x04, ///<错误事件掩码
    kEventMask    = 0xff  ///<事件掩码
};

/// 定义跨平台的socket
#if defined(_WIN32)
    typedef ::SOCKET handle_t;
#elif defined(__linux__)
    typedef int handle_t;
#else
#error "failure"
#endif // _WIN32

/// 事件处理器
class EventHandler
{
public:

    /// 获取该handler所对应的句柄
    virtual handle_t GetHandle() const = 0;

    /// 处理读事件的回调函数
    virtual void HandleRead() {}

    /// 处理写事件的回调函数
    virtual void HandleWrite() {}

    /// 处理出错事件的回调函数
    virtual void HandleError() {}

protected:

    /// 构造函数,只能子类调
    EventHandler() {}

    /// 析构函数,只能子类调
    virtual ~EventHandler() {}
};

/// reactor的实现类
class ReactorImplementation;

/// reactor反应器
class Reactor
{
public:

    /// 构造函数
    Reactor();

    /// 析构函数
    ~Reactor();

    /// 向reactor中注册关注事件evt的handler(可重入)
    /// @param  handler 要注册的事件处理器
    /// @param  evt     要关注的事件
    /// @retval 0       注册成功
    /// @retval -1      注册出错
    int RegisterHandler(EventHandler * handler, event_t evt);

    /// 从reactor中移除handler
    /// @param  handler 要移除的事件处理器
    /// @retval 0       移除成功
    /// @retval -1      移除出错
    int RemoveHandler(EventHandler * handler);

    /// 处理事件,回调注册的handler中相应的事件处理函数
    /// @param  timeout 超时时间(毫秒)
    void HandleEvents(int timeout = 0);

private:

    /// 禁止拷贝构造和赋值操作
    Reactor(const Reactor &);
    Reactor & operator=(const Reactor &);

private:

    ReactorImplementation* m_reactor_impl; ///< reactor的实现类
};
} // namespace reactor
#endif // _REACTOR_H_