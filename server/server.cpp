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

#define PORT 7302
#define BUFFER_SIZE 1024

class Server
{
public:
    Server(const int port, const int buffer_size);
    void set_new_port(const int& port);
    void create_socket();
    void bind_to_port(); 
    int listen_client() const;
    void handle_events(std::atomic<bool> &atomic_bool);


//for test
    int get_port();

private:
    bool parse_message(std::string, int&);
    void handle_write(std::atomic<bool> &atomic_bool);
    void timer();
    bool isPortAvailable(int port);

private: 
    std::unordered_map<int, int> m_fd_map;
    int m_server_socket, m_port, m_buffer_size, m_epollfd;
    struct sockaddr_in m_address;
    struct epoll_event m_event;

    enum M_CLIENT_SOCKET { FIRST = 0 };
};

Server::Server(const int port, const int buffer_size) : m_port{port}, 
                                                        m_buffer_size{buffer_size}
{    
}

bool Server::isPortAvailable(int port) {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Порт isnt avaiable\n";
        close(sockfd);
        return false;
    } else {
        std::cout << "port is available" << std::endl;
        close(sockfd);
        return true;
    }
}

void Server::set_new_port(const int& port)
{
    m_port = port;
}

void Server::create_socket()
{
    m_server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(m_server_socket == -1)
    {
        perror("socket");
    }

    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = INADDR_ANY;

    m_epollfd = epoll_create1(0);
    if(m_epollfd == -1)
    {
        perror("epoll_create1");
    }

    m_event.events = EPOLLIN;
    m_event.data.fd = m_server_socket;
}

void Server::bind_to_port()
{
    m_address.sin_port = htons(m_port);
    std::cout << "BIND --------- " << std::endl;
    std::cout << m_server_socket << std::endl;
    std::cout << m_port << std::endl;
    std::cout << "BIND --------- " << std::endl;
    if(bind(m_server_socket, reinterpret_cast<struct sockaddr *>(&m_address), sizeof(m_address)))
    {
        perror("bind");
    }
}

int generate_random_port() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(1024, 65535);
    return distribution(gen);
}

int Server::get_port()
{
    socklen_t len;
    int port;
    len = sizeof(m_address);
    getpeername(m_server_socket, (struct sockaddr*)&m_address, &len);
    if (m_address.sin_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&m_address;
        port = ntohs(s->sin_port);
    }
    return port;
}

int Server::listen_client() const
{
   return listen(m_server_socket, 3); 
}

void Server::timer()
{
    std::this_thread::sleep_for(std::chrono::seconds(45));
}

void Server::handle_events(std::atomic<bool> &atomic_bool)
{
    std::cout << "handle_events()" << std::endl;
    struct epoll_event events;
    char buffer[m_buffer_size];
    // std::future<void> future = std::async(&Server::timer, this);
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_server_socket, &m_event) == -1) {
        perror("epoll_ctl");
        return;
    }
    while(!atomic_bool) {
        int num_events = epoll_wait(m_epollfd, &events, 1, -1);
        std::cout << "epoll" << std::endl;
        if (num_events == -1) {
            perror("epoll_wait");
            return; 
        }
        if (events.data.fd == m_server_socket) {
            struct sockaddr_in client_address;
            std::cout << "in" << std::endl;
            socklen_t client_len = sizeof(client_address);
            int client_socket = accept(m_server_socket, reinterpret_cast<struct sockaddr *>(&client_address), &client_len);
            m_fd_map[M_CLIENT_SOCKET::FIRST] = client_socket;
            if (client_socket == -1) {
                perror("accept");
            }
            std::cout << "New client connected" << std::endl;
            // add new docket in epoll
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = client_socket;
            if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                perror("epoll_ctl");
                close(client_socket);
            }
            std::thread t1([this, &atomic_bool]{handle_write(atomic_bool);});
            t1.detach();
        }
        else 
        {
            int client_socket = events.data.fd;
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            int newPort;
            // if (!future.valid()) {
            //     std::cout << "change port ... " << std::endl;
            //     m_fd_map[M_CLIENT_SOCKET::FIRST] = -1;
            //     for(;;)
            //     {
            //         m_port = generate_random_port();
            //         if(isPortAvailable(m_port)){ break; }
            //     }
            //     std::string new_port_message = "newport-" +  std::to_string(m_port); 
            //     send(client_socket, new_port_message.c_str(), strlen(new_port_message.c_str()), 0); 
            //     epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_server_socket, nullptr);
            //     close(m_server_socket);
            //     break;
            // }
            if(parse_message(std::string(buffer, bytes_received), newPort))
            {
                std::cout << "if into handle_events, parse_message = TRUE" << std::endl;
                if(isPortAvailable(newPort))
                {
                    m_fd_map[M_CLIENT_SOCKET::FIRST] = -1;
                    m_port = newPort;
                    std::string new_port_message = "newport-" +  std::to_string(m_port); 
                    send(client_socket, new_port_message.c_str(), strlen(new_port_message.c_str()), 0); 
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_server_socket, nullptr); // Удаляем серверный сокет из epoll
                    close(m_server_socket);
                    break;
                }
                else
                {
                    std::string new_port_message = "fail";
                    send(client_socket, new_port_message.c_str(), strlen(new_port_message.c_str()), 0); 
                }
                 
            }
            if (bytes_received == -1) {
                perror("recv");
            } else if (bytes_received == 0) {
                std::cout << "Client disconnected" << std::endl;
                close(client_socket);
            } else {
                std::cout << "Received data from client: " << std::string(buffer, bytes_received) << std::endl;
            }
        }
    }
}

void Server::handle_write(std::atomic<bool> &atomic_bool)
{
    while(!atomic_bool)
    {   
        if (m_fd_map[M_CLIENT_SOCKET::FIRST] > 0)
        {   
            std::string line;
            std::getline(std::cin, line);
            int intPort;
            if(parse_message(line, intPort))
            {
                if(isPortAvailable(intPort))
                {
                    m_port = intPort;
                    const char* cline = line.c_str(); // Получаем указатель на строку
                    int len = strlen(cline);
                    send(m_fd_map[M_CLIENT_SOCKET::FIRST], cline, len, 0);
                    atomic_bool = true;
                }
            }
            else
            {
                const char* cline = line.c_str(); // Получаем указатель на строку
                int len = strlen(cline); // Получаем длину строки
                int bytes_sent = send(m_fd_map[M_CLIENT_SOCKET::FIRST], cline, len, 0); // Отправляем данные
                if(bytes_sent < 0)
                {
                    std::cout << "line wasnt send (handler_write)" << std::endl;
                }
            }
            
            // Очищаем буфер после отправки данных
            line.clear();
        }
        else
        {
            std::cout << "we close socket because we resieve a command for change port" << std::endl;
            return;
        }
    }
}


bool Server::parse_message(std::string message, int& intPort)
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

    if(message == "newport-" && port.size() < 6)
    {
        intPort = std::stoi(port);
        return (intPort > 1024 && intPort < 65535);
    }
    std::cout << "port does not exist or message does not equal to command" << std::endl; 
    return false;
}

/**
 * void schedulr(s)
 * {
 *      std::atomic<bool> stop(false);
 *      std::future<void> f1 = std::async([s, &stop]{s->handle_events(stop);});
 *      std::sleep(45);
 *      stop = true;
 *      int port = getport();
 *      s->setport(port);
 *      
        f1.get();
 * }
*/

// void schedular(Server *s)
// {
//     std::atomic<bool> stop(false);
//     int newPort = generate_random_port();
//     std::future<void> f1 = std::async([s, &stop]{s->handle_events(stop);});
//     std::this_thread::sleep_for(std::chrono::seconds(45));
//     s->set_new_port(newPort);
//     stop = true;
// }

int main()
{
    Server *s = new Server(PORT, BUFFER_SIZE);
    
    for(;;)
    {   
        std::atomic<bool> stop(false);
        s->create_socket();
        s->bind_to_port();
        std::cout << "PORT -> " << s->get_port() << std::endl;
        if(s->listen_client() < 0)
        {
            std::cout << "xnj nj yt nj" << std::endl;
        }
        std::cout << "1" << std::endl;
        std::future<void> f1 = std::async([s, &stop]{s->handle_events(stop);});
        // schedular(s);
        f1.get();
    }      
}


//############################################################################################################################################

// namespace
// {
//     std::unordered_map<int, int> fd_map;
// }

// bool parse_message(std::string, int&);

// int handle_events(int epoll_fd, int server_socket) {
//     struct epoll_event events;
//     char buffer[BUFFER_SIZE];
//     for(;;) {
//         int num_events = epoll_wait(epoll_fd, &events, 1, -1);
//         if (num_events == -1) {
//             perror("epoll_wait");
//             return -1; 
//         }
//         if (events.data.fd == server_socket) {
//             // Событие на серверном сокете - новое соединение
//             struct sockaddr_in client_address;
//             socklen_t client_len = sizeof(client_address);
//             int client_socket = accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_address), &client_len);
//             ::fd_map[CLIENT_SOCKET] = client_socket;
//             if (client_socket == -1) {
//                 perror("accept");
//             }

//             std::cout << "New client connected" << std::endl;

//             // Добавляем новый сокет в epoll
//             struct epoll_event event;
//             event.events = EPOLLIN; // Ожидаем чтение
//             event.data.fd = client_socket;
//             if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
//                 perror("epoll_ctl");
//                 close(client_socket);
//             }
//         }
//         else 
//         {
//             // Событие на клиентском сокете - данные готовы к чтению
//             int client_socket = events.data.fd;
//             ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
//             int newPort;
//             if(parse_message(std::string(buffer, bytes_received), newPort))
//             {
//                 std::cout << "if into handle_events, parse_message = TRUE" << std::endl;
//                 ::fd_map[CLIENT_SOCKET] = -1;
//                 return newPort; 
//             }
//             if (bytes_received == -1) {
//                 perror("recv");
//                 // Обработка ошибок чтения
//             } else if (bytes_received == 0) {
//                 // Соединение закрыто клиентом
//                 std::cout << "Client disconnected" << std::endl;
//                 close(client_socket);
//             } else {
//                 std::cout << "Received data from client: " << std::string(buffer, bytes_received) << std::endl;
//             }
//         }
//     }
//     return -1;
// }

// void handle_write()
// {
//     for(;;)
//     {   
//         if (::fd_map[CLIENT_SOCKET] > 0)
//         {
//             std::string line;
//             std::getline(std::cin, line);
//             const char* cline = line.c_str(); // Получаем указатель на строку
//             int len = strlen(cline); // Получаем длину строки
//             int bytes_sent = send(::fd_map[CLIENT_SOCKET], cline, len, 0); // Отправляем данные
//             if(bytes_sent < 0)
//             {
//                 std::cout << "line wasnt send (handler_write)" << std::endl;
//             }
//         }
//         else
//         {
//             std::cout << "we close socket because we resieve a command for change port" << std::endl;
//             return;
//         }
//         // Очищаем буфер после отправки данных
//         line.clear();
//     }
// }


// bool parse_message(std::string message, int& intPort)
// {
//     //port for change
//     std::string port;
//     //remove spaces
//     message.erase(std::remove(message.begin(), message.end(), ' '), message.end());
//     //to lower case
//     std::transform(message.begin(), message.end(), message.begin(), [](unsigned char c) {
//         return std::tolower(c);
//     });
//     //remove digits
//     message.erase(std::remove_if(message.begin(), message.end(), [&port](unsigned char c) {
//         if(isdigit(c))
//         {
//             port.push_back(c);
//         }
//         return std::isdigit(c);
//     }), message.end());

//     //testing 
//     std::cout << "test port - " << port << std::endl;

//     //65535
//     if(message == "newport-" && port.size() < 6)
//     {
//         intPort = std::stoi(port);
//         return (intPort > 1024 && intPort < 65535);
//     }
//     std::cout << "port does not exist or message does not equal to command" << std::endl; 
//     return false;
// }

// int main() {
//     int server_socket = socket(PF_INET, SOCK_STREAM, 0);
//     if (server_socket == -1) {
//         perror("socket");
//         return 1;
//     }

//     struct sockaddr_in address;
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1) {
//         perror("bind");
//         return 1;
//     }

//     if (listen(server_socket, 3) == -1) {
//         perror("listen");
//         return 1;
//     }

//     int epoll_fd = epoll_create1(0);
//     if (epoll_fd == -1) {
//         perror("epoll_create1");
//         return 1;
//     }

//     struct epoll_event event;
//     event.events = EPOLLIN; // Ожидаем входные данные на серверном сокете
//     event.data.fd = server_socket;
//     if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
//         perror("epoll_ctl");
//         return 1;
//     }
//
//     std::future<int> f1 = std::async([epoll_fd, server_socket]{return handle_events(epoll_fd, server_socket);});
//     std::future<void> f2 = std::async([server_socket]{handle_write();});
//     auto newPort = f1.get();
//     f2.get();
//     close(server_socket);
//     return 0;
// }
