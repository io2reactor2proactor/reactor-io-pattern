
#include <assert.h>
#include "reactor.h"
#include "eventdemultiplexer.h"

/// @file   reactor.cpp
/// @brief
/// @author lovezhangkai@foxmail
/// @date   2013-10-1

namespace reactor
{
/// reactor的实现类
class ReactorImplementation
{
public:

    /// 构造函数
    ReactorImplementation();

    /// 析构函数
    ~ReactorImplementation();

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
    void HandleEvents(int timeout);

private:

    EventDemultiplexer*                m_demultiplexer;  ///< 事件分离器
    std::map<handle_t, EventHandler*>  m_handlers;       ///< 句柄与事件处理器映射表 
};

///////////////////////////////////////////////////////////////////////////////

/// 构造函数
Reactor::Reactor()
{
    m_reactor_impl = new ReactorImplementation();
}

/// 析构函数
Reactor::~Reactor()
{
    delete m_reactor_impl;
}

/// 向reactor中注册关注事件evt的handler(可重入)
/// @param  handler 要注册的事件处理器
/// @param  evt     要关注的事件
/// @retval 0       注册成功
/// @retval -1      注册出错
int Reactor::RegisterHandler(EventHandler * handler, event_t evt)
{
    return m_reactor_impl->RegisterHandler(handler, evt);
}

/// 从reactor中移除handler
/// @param  handler 要移除的事件处理器
/// @retval 0       移除成功
/// @retval -1      移除出错
int Reactor::RemoveHandler(EventHandler * handler)
{
    return m_reactor_impl->RemoveHandler(handler);
}

/// 处理事件,回调注册的handler中相应的事件处理函数
/// @param  timeout 超时时间(毫秒)
void Reactor::HandleEvents(int timeout)
{
    m_reactor_impl->HandleEvents(timeout);
}

///////////////////////////////////////////////////////////////////////////////

/// 构造函数
ReactorImplementation::ReactorImplementation()
{
#if defined(_WIN32)
    m_demultiplexer = new SelectDemultiplexer(); ///windows平台 select IO多路复用模型
#elif defined(__linux__)
    m_demultiplexer = new EpollDemultiplexer(); ///linux平台 epoll IO多路复用模型
#else
#error "failure"
#endif // _WIN32
}

/// 析构函数
ReactorImplementation::~ReactorImplementation()
{
    delete m_demultiplexer;
}

/// 向reactor中注册关注事件evt的handler(可重入)
/// @param  handler 要注册的事件处理器
/// @param  evt     要关注的事件
/// @retval 0       注册成功
/// @retval -1      注册出错
int ReactorImplementation::RegisterHandler(EventHandler* handler, event_t evt)
{
    handle_t handle = handler->GetHandle();
    std::map<handle_t, EventHandler*>::iterator it = m_handlers.find(handle);
    if (it == m_handlers.end())
    {
        m_handlers[handle] = handler;
    }
    return m_demultiplexer->RequestEvent(handle, evt);
}

/// 从reactor中移除handler
/// @param  handler 要移除的事件处理器
/// @retval 0       移除成功
/// @retval -1      移除出错
int ReactorImplementation::RemoveHandler(EventHandler * handler)
{
    handle_t handle = handler->GetHandle();
    m_handlers.erase(handle);
    return m_demultiplexer->UnrequestEvent(handle);
}

/// 处理事件,回调注册的handler中相应的事件处理函数
/// @param  timeout 超时时间(毫秒)
void ReactorImplementation::HandleEvents(int timeout)
{
    m_demultiplexer->WaitEvents(&m_handlers);
}
} // namespace reactor