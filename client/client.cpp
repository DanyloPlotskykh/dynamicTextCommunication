#include "client.hpp"

Client::Client(std::string addr, int port) : m_addr(std::move(addr)),
                                               m_port(std::move(port))
{
}

void Client::create_socket()
{
    if(m_sock != 0)
    {
        close(m_sock);
    }
    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(m_sock < 0)
    {
        perror("socket");
    }
    m_epollfd = epoll_create1(0);
        
    m_event.events = EPOLLIN;
    m_event.data.fd = m_sock;
    std::cout << "before before" << std::endl;
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_sock, &m_event) == -1)
    {
        perror("epoll_ctl");
    }
}

void Client::bind_to_server()
{
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_port = htons(m_port);

    if (inet_pton(PF_INET, m_addr.c_str(), &m_serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
    }
}

bool Client::isPortAvailable(int port) {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        close(sockfd);
        return false;
    } else {
        close(sockfd);
        return true;
    }
}

bool Client::connect_to_server()
{
    if (connect(m_sock, (struct sockaddr *)&m_serv_addr, sizeof(m_serv_addr)) < 0) {
        perror("connect");
        return false;
    }
    return true;
}

void Client::handle_events(std::atomic<bool> &stop, std::atomic<bool> &port_is_changed)
{
    struct epoll_event events;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = 0;
    int remaining_bytes = 0;
    while(!stop)
    {
        int num_events = epoll_wait(m_epollfd, &events, 1, -1);
        if(num_events == -1)
        {
            perror("epoll_wait");
        }
        if(events.data.fd == m_sock)
        {
            ssize_t bytes = recv(m_sock, buffer, BUFFER_SIZE, 0);
            if(bytes == -1)
            {
                perror("recv");
            }
            else if(bytes == 0)
            {
                std::cout << "Server closed connection" << std::endl;
                break;
            }
            else
            {
                int port;
                if(parse_message(std::string(buffer, bytes), port))
                {
                    m_port = port;
                    port_is_changed = true;
                    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sock, nullptr);
                    close(m_epollfd);
                    close(m_sock);
                    break;
                }
                else
                {
                    std::cout << std::string(buffer, bytes) << std::endl;
                }
                
            }
        }
    } 
}

void Client::handle_write(std::atomic<bool> &stop, std::atomic<bool> &port_is_changed)
{
    while(!stop)
    {
        std::string line;
        std::getline(std::cin, line);
        auto cline = line.c_str();
        int len = strlen(cline);
        int tmp;
        if(parse_message(line, tmp))
        {
            m_port = tmp;
            int bytes_sent = send(m_sock, cline, len, 0);
            if(bytes_sent < 0)
            {
                std::cout << "line does not sent (handler_write)" << std::endl;
            }
            port_is_changed = true;
            break;
        }
        int bytes_sent = send(m_sock, cline, len, 0);
        if(bytes_sent < 0)
        {
            std::cout << "line does not sent (handler_write)" << std::endl;
        }
        line.clear();
    }
}

bool Client::parse_message(std::string message, int& intPort)
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

    if(message == "newport-" && port.size() < 6)
    {
        intPort = std::stoi(port);
        return (intPort > 1024 && intPort < 65535);
    }
    return false;
}

int Client::generate_random_port() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(1024, 65535);
    return distribution(gen);
}