
#include <errno.h>
#include <assert.h>
#include <vector>
#include "eventdemultiplexer.h"

/// @file   event_demultiplexer.cpp
/// @brief
/// @author lovezhangkai@foxmail
/// @date   2013-10-1

namespace reactor
{
#if defined(_WIN32)
//#pragma comment(lib, "Ws2_32.lib")
/// 构造函数
SelectDemultiplexer::SelectDemultiplexer()
{
    this->Clear();
}

/// 获取有事件发生的所有句柄以及所发生的事件
/// @param  events  获取的事件
/// @param  timeout 超时时间
/// @retval = 0   没有发生事件的句柄(超时)
/// @retval > 0   发生事件的句柄个数
/// @retval < 0   发生错误
int SelectDemultiplexer::WaitEvents(std::map<handle_t, EventHandler *> * handlers, int timeout)
{
    /// 设置超时时间
    m_timeout.tv_sec = timeout / 1000;
    m_timeout.tv_usec = timeout % 1000 * 1000;
    int max_fd = handlers->rbegin()->first;
    int ret = select(max_fd + 1, &m_read_set, &m_write_set, &m_except_set, &m_timeout);
    if (ret <= 0)
    {
        return ret;
    }
    /// 遍历注册的事件表, 查看是否有事件发生
    std::map<handle_t, EventHandler *>::iterator it = handlers->begin();
    while (it != handlers->end())
    {
        if (FD_ISSET(it->first, &m_except_set))
        {
            it->second->HandleError();
            FD_CLR(it->first, &m_read_set);
            FD_CLR(it->first, &m_write_set);
        }
        else
        {
            if (FD_ISSET(it->first, &m_read_set))
            {
                it->second->HandleRead();
                FD_CLR(it->first, &m_read_set);
            }
            if (FD_ISSET(it->first, &m_write_set))
            {
                it->second->HandleWrite();
                FD_CLR(it->first, &m_write_set);
            }
        }
        FD_CLR(it->first, &m_except_set);
        ++it;
    }
    return ret;
}

/// 设置句柄handle关注evt事件
/// @retval = 0 设置成功
/// @retval < 0 设置出错
int SelectDemultiplexer::RequestEvent(handle_t handle, event_t evt)
{
    if (evt & kReadEvent)
    {
        FD_SET(handle, &m_read_set);
    }
    if (evt & kWriteEvent)
    {
        FD_SET(handle, &m_write_set);
    }
    FD_SET(handle, &m_except_set);
    return 0;
}

/// 撤销句柄handle对事件evt的关注
/// @retval = 0 撤销成功
/// @retval < 0 撤销出错
int SelectDemultiplexer::UnrequestEvent(handle_t handle)
{
    FD_CLR(handle, &m_read_set);
    FD_CLR(handle, &m_write_set);
    FD_CLR(handle, &m_except_set);
    return 0;
}

/// 初始化文件描述符集合
void SelectDemultiplexer::Clear()
{
    FD_ZERO(&m_read_set);
    FD_ZERO(&m_write_set);
    FD_ZERO(&m_except_set);
}
#elif defined(__linux__)
/// 构造函数
EpollDemultiplexer::EpollDemultiplexer()
{
    m_epoll_fd = ::epoll_create(FD_SETSIZE);
    assert(m_epoll_fd != -1);
    m_fd_num = 0;
}

/// 析构函数
EpollDemultiplexer::~EpollDemultiplexer()
{
    ::close(m_epoll_fd);
}

/// 获取有事件发生的所有句柄以及所发生的事件
/// @param  events  获取的事件
/// @param  timeout 超时时间
/// @retval = 0   没有发生事件的句柄(超时)
/// @retval > 0   发生事件的句柄个数
/// @retval < 0   发生错误
int EpollDemultiplexer::WaitEvents(std::map<handle_t, EventHandler *> * handlers, int timeout)
{
    std::vector<epoll_event> ep_evts(m_fd_num);
    int num = epoll_wait(m_epoll_fd, &ep_evts[0], ep_evts.size(), timeout);
    if (num > 0)
    {
        for (int idx = 0; idx < num; ++idx)
        {
            handle_t handle = ep_evts[idx].data.fd;
            assert(handlers->find(handle) != handlers->end());
            if ((ep_evts[idx].events & EPOLLERR) ||
                    (ep_evts[idx].events & EPOLLHUP))
            {
                (*handlers)[handle]->HandleError();
            }
            else
            {
                if (ep_evts[idx].events & EPOLLIN)
                {
                    (*handlers)[handle]->HandleRead();
                }
                if (ep_evts[idx].events & EPOLLOUT)
                {
                    (*handlers)[handle]->HandleWrite();
                }
            }
        }
    }
    return num;
}

/// 设置句柄handle关注evt事件
/// @retval = 0 设置成功
/// @retval < 0 设置出错
int EpollDemultiplexer::RequestEvent(handle_t handle, event_t evt)
{
    epoll_event ep_evt;
    ep_evt.data.fd = handle;
    ep_evt.events = 0;

    if (evt & kReadEvent)
    {
        ep_evt.events |= EPOLLIN;
    }
    if (evt & kWriteEvent)
    {
        ep_evt.events |= EPOLLOUT;
    }
    ep_evt.events |= EPOLLONESHOT;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, handle, &ep_evt) != 0)
    {
        if (errno == ENOENT)
        {
            if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, handle, &ep_evt) != 0)
            {
                return -errno;
            }
            ++m_fd_num;
        }
    }
    return 0;
}

/// 撤销句柄handle对事件evt的关注
/// @retval = 0 撤销成功
/// @retval < 0 撤销出错
int EpollDemultiplexer::UnrequestEvent(handle_t handle)
{
    epoll_event ep_evt;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, handle, &ep_evt) != 0)
    {
        return -errno;
    }
    --m_fd_num;
    return 0;
}
#else
#error "failure"
#endif // _WIN32
} // namespace reactor