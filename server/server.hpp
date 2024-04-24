#pragma once
#include <iostream>
#include <future>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cctype>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

#define BUFFER_SIZE 1024

class Server
{
public:
    Server(const int port, const int buffer_size);
    void create_socket();
    void bind_to_port(); 
    int listen_client() const;
    void handle_events(std::atomic<bool> &stop);

private:
    bool parse_message(std::string, int&);
    void handle_write(std::atomic<bool> &stop);
    bool isPortAvailable(int port);

private: 
    std::unordered_map<int, int> m_fd_map;
    int m_server_socket, m_port, m_buffer_size, m_epollfd;
    struct sockaddr_in m_address;
    struct epoll_event m_event;

    enum M_CLIENT_SOCKET { FIRST = 0 };
};
