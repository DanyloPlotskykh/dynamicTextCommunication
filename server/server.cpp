#include "server.hpp"

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

int Server::listen_client() const
{
   return listen(m_server_socket, 3); 
}

void Server::handle_events(std::atomic<bool> &stop)
{
    std::cout << "handle_events()" << std::endl;
    struct epoll_event events;
    char buffer[m_buffer_size];
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_server_socket, &m_event) == -1) {
        perror("epoll_ctl");
        return;
    }
    while(!stop) {
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
            std::thread t1([this, &stop]{handle_write(stop);});
            t1.detach();
        }
        else 
        {
            int client_socket = events.data.fd;
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            int newPort;
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

void Server::handle_write(std::atomic<bool> &stop)
{
    while(!stop)
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
                    stop = true;
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
    return false;
}