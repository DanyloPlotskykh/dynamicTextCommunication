#include <iostream>
#include <future>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define SERVER_IP "127.0.0.1"
#define PORT 7301
#define BUFFER_SIZE 1024
#define MAX_EVENTS 5

void epoll_loop(int epoll_fd, int sock)
{
    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = 0;
    int remaining_bytes = 0;
    while(1)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
        }

        for(int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == sock) {
                ssize_t bytes = recv(sock, buffer, BUFFER_SIZE, 0);
                if (bytes == -1) {
                    perror("recv");
                    // Handle error
                } else if (bytes == 0) {
                    // Connection closed
                    // close(epoll_fd);
                    // close(sockfd);
                } else {
                    std::cout << std::string(buffer, bytes) << std::endl;
                }
            }
        }
    }
}

void handler_write(int sock)
{
    while(1)
    {
        std::string line;
        std::getline(std::cin, line);
        auto cline = line.c_str(); // Получаем указатель на строку
        int len = strlen(cline); // Получаем длину строки
        int bytes_sent = send(sock, cline, len, 0); // Отправляем данные
        if(bytes_sent < 0)
        {
            std::cout << "line does not sent (handler_write)" << std::endl;
        }
        // Очищаем буфер после отправки данных
        line.clear();
    }
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
    {
        throw std::runtime_error("socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(PF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        throw std::runtime_error("epoll_fd");
    }
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sock;
    std::cout << "before before" << std::endl;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event) == -1)
    {
        throw std::runtime_error("epoll_ctl == -1");
    }
    std::cout << "before" << std::endl;
    std::future<void> f1 = std::async([epoll_fd, sock]{epoll_loop(epoll_fd, sock);});
    std::future<void> f2 = std::async([sock]{handler_write(sock);});
    f1.get();
    f2.get();
}