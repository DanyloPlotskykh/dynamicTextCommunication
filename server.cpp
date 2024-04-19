#include <iostream>
#include <future>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define PORT 7300
#define BUFFER_SIZE 1024
#define MAX_EVENTS 5

void handle_read(void *arg/*, epoll maybe*/)
{
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE] = {0};
        read(client_socket, buffer, BUFFER_SIZE);
        // if(strcmp(buffer, "\n") == 0) {continue;}
        std::cout << std::string(buffer) << std::endl;
        memset(buffer, 0, BUFFER_SIZE);
}

int main(int args, char **argv)
{
    int fd, new_socket, epoll_fd;
    struct sockaddr_in address;
    struct epoll_event event, events[MAX_EVENTS];
    int len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == 0) {
        throw std::runtime_error("socket failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr *)&address, len) < 0) {
        throw std::runtime_error("bind failed");
    }

    if (listen(fd, 3) < 0) {
        throw std::runtime_error("listen");
    }

    // Принятие входящего соединения
    if ((new_socket = accept(fd, (struct sockaddr *)&address, (socklen_t*)&len)) < 0) {
        throw std::runtime_error("accept");
    }
    std::future<void> f = std::async([new_socket]{handle_read((void *)&new_socket);});
    std::cout << "All good yet" << std::endl;
}