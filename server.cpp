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

#define PORT 7301
#define BUFFER_SIZE 1024
#define MAX_EVENTS 5
#define CLIENT_SOCKET 1

namespace
{
    std::unordered_map<int, int> fd_map;
}

bool parse_message(std::string, int&);

int handle_events(int epoll_fd, int server_socket) {
    struct epoll_event events;
    char buffer[BUFFER_SIZE];
    for(;;) {
        int num_events = epoll_wait(epoll_fd, &events, 1, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            return -1; 
        }
        if (events.data.fd == server_socket) {
            // Событие на серверном сокете - новое соединение
            struct sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            int client_socket = accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_address), &client_len);
            ::fd_map[CLIENT_SOCKET] = client_socket;
            if (client_socket == -1) {
                perror("accept");
            }

            std::cout << "New client connected" << std::endl;

            // Добавляем новый сокет в epoll
            struct epoll_event event;
            event.events = EPOLLIN; // Ожидаем чтение
            event.data.fd = client_socket;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                perror("epoll_ctl");
                close(client_socket);
            }
        }
        else 
        {
            // Событие на клиентском сокете - данные готовы к чтению
            int client_socket = events.data.fd;
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            int newPort;
            if(parse_message(std::string(buffer, bytes_received), newPort))
            {
                std::cout << "if into handle_events, parse_message = TRUE" << std::endl;
                ::fd_map[CLIENT_SOCKET] = -1;
                return newPort; 
            }
            if (bytes_received == -1) {
                perror("recv");
                // Обработка ошибок чтения
            } else if (bytes_received == 0) {
                // Соединение закрыто клиентом
                std::cout << "Client disconnected" << std::endl;
                close(client_socket);
            } else {
                std::cout << "Received data from client: " << std::string(buffer, bytes_received) << std::endl;
            }
        }
    }
    return -1;
}

void handle_write()
{
    for(;;)
    {
        std::string line;
        std::getline(std::cin, line);
        const char* cline = line.c_str(); // Получаем указатель на строку
        int len = strlen(cline); // Получаем длину строки
        if (::fd_map[CLIENT_SOCKET] > 0)
        {
            int bytes_sent = send(::fd_map[CLIENT_SOCKET], cline, len, 0); // Отправляем данные
            if(bytes_sent < 0)
            {
                std::cout << "line does not sent (handler_write)" << std::endl;
            }
        }
        else
        {
            std::cout << "we close socket because we resieve a command for change port" << std::endl;
            return;
        }
        // Очищаем буфер после отправки данных
        line.clear();
    }
}


bool parse_message(std::string message, int& intPort)
{
    //port for change
    std::string port;
    //remove spaces
    message.erase(std::remove(message.begin(), message.end(), ' '), message.end());
    //to lower case
    std::transform(message.begin(), message.end(), message.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    //remove digits
    message.erase(std::remove_if(message.begin(), message.end(), [&port](unsigned char c) {
        if(isdigit(c))
        {
            port.push_back(c);
        }
        return std::isdigit(c);
    }), message.end());

    //testing 
    std::cout << "test port - " << port << std::endl;

    //65535
    if(message == "newport-" && port.size() < 6)
    {
        intPort = std::stoi(port);
        return (intPort > 1024 && intPort < 65535);
    }
    std::cout << "port does not exist or message does not equal to command" << std::endl; 
    return false;
}

int main() {
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_socket, 3) == -1) {
        perror("listen");
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN; // Ожидаем входные данные на серверном сокете
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    std::future<int> f1 = std::async([epoll_fd, server_socket]{return handle_events(epoll_fd, server_socket);});
    std::future<void> f2 = std::async([server_socket]{handle_write();});
    auto newPort = f1.get();
    f2.get();
    close(server_socket);
    return 0;
}
