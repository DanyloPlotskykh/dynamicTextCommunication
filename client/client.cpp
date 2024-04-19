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
#define PORT 7300
#define BUFFER_SIZE 1024

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
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

    // recv(sock, buffer, BUFFER_SIZE, 0);
    strcpy(buffer,"Hello from client!!!!");
    send(sock, buffer, strlen(buffer), 0);
    std::cout << std::string(buffer) << std::endl;

}