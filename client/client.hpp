#pragma once
#include <iostream>
#include <future>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cctype>
#include <string>
#include <algorithm>
#include <thread>
#include <random>

#define BUFFER_SIZE 1024

class Client
{
public:
    Client(std::string addr, int port);
    void create_socket();
    void bind_to_server();
    bool connect_to_server();
    void handle_events(std::atomic<bool> &stop, std::atomic<bool> &port_is_changed);
    void handle_write(std::atomic<bool> &stop, std::atomic<bool> &port_is_changed);

    friend void schedular(Client *cl, std::atomic<bool> &stop, std::atomic<bool> &port_is_changed);

private:
    
    bool parse_message(std::string message, int& intPort);
    bool isPortAvailable(int port);
    int generate_random_port();
private:
    const std::string m_addr;
    int m_port, m_epollfd, m_sock = 0;
    struct sockaddr_in m_serv_addr;
    struct epoll_event m_event;
};