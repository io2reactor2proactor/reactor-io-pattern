This is io design pattern --- reactor

Reactor is Event Driven and use os api(select, poll, epoll, kqueue, iocp etc..) to dispatch socket event.

Programming only need register event source socket happend read, write or except and concrete event handler to Reactor.

Reactor dispatch automatically socket event hanppend to event-handler registered.

time_server.cpp and time_client.cpp only a example.

other files are realize reactor files. 
