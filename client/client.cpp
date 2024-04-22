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

std::string ADDR = "127.0.0.1";
int PORT = 7302;
#define BUFFER_SIZE 1024
#define MAX_EVENTS 5

class Client
{
public:
    Client(std::string addr, int port);
    void create_socket();
    void bind();
    bool connect_to_server();
    void handle_events();
    void handle_write();

private:
    bool parse_message(std::string message, int& intPort);

private:
    const std::string m_addr;
    int m_port, m_epollfd, m_sock = 0;
    struct sockaddr_in m_serv_addr;
    struct epoll_event m_event;
};

Client::Client(std::string addr, int port) : m_addr(std::move(addr)),
                                               m_port(std::move(port))
{
}

void Client::create_socket()
{
    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(m_sock < 0)
    {
        std::cout << "socket failure" << std::endl;
    }
    m_epollfd = epoll_create1(0);
        
    m_event.events = EPOLLIN;
    m_event.data.fd = m_sock;
    std::cout << "before before" << std::endl;
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_sock, &m_event) == -1)
    {
        std::cout << "epoll_ctl()" << std::endl;
    }
}

void Client::bind()
{
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_port = htons(m_port); // Установка порта

    if (inet_pton(PF_INET, m_addr.c_str(), &m_serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
    }
}

bool Client::connect_to_server()
{
    // if (m_sock != 0) {
    //     close(m_sock); // Закрыть предыдущее соединение
    // }

    // m_sock = socket(PF_INET, SOCK_STREAM, 0); // Создать новый сокет
    // if (m_sock < 0) {
    //     std::cout << "socket failure" << std::endl;
    // }

    // Установка нового порта в структуре адреса
    // m_serv_addr.sin_port = htons(m_port);

    if (connect(m_sock, (struct sockaddr *)&m_serv_addr, sizeof(m_serv_addr)) < 0) {
        perror("connect");
        return -1;
    }
    return true;
}

void Client::handle_events()
{
    struct epoll_event events;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = 0;
    int remaining_bytes = 0;
    for(;;)
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
            }
            else
            {
                int port;
                if(parse_message(std::string(buffer, bytes), port))
                {
                    m_port = port;
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
        else 
        {
            std::cout << "else" << std::endl;
        }
    } 
}

void Client::handle_write()
{
    for(;;)
    {
        std::string line;
        std::getline(std::cin, line);
        auto cline = line.c_str(); // Получаем указатель на строку
        int len = strlen(cline); // Получаем длину строки
        int tmp;
        if(parse_message(line, tmp))
        {
            std::cout << "TRUE" << std::endl;
            m_port = tmp;
            int bytes_sent = send(m_sock, cline, len, 0);
            if(bytes_sent < 0)
            {
                std::cout << "line does not sent (handler_write)" << std::endl;
            }
            break;
            return;
        }
        int bytes_sent = send(m_sock, cline, len, 0); // Отправляем данные
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
    std::cout << "message before parse -- " << message << std::endl;
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

int main()
{
    Client *cl = new Client(ADDR, PORT);
    for(;;)
    {
        cl->create_socket();
        cl->bind();
        if(!cl->connect_to_server())
        {
            break;
        }
        std::future<void> f1 = std::async([cl]{cl->handle_events();});
        std::future<void> f2 =  std::async([cl]{cl->handle_write();});
        f1.get();
        f2.get();
    }
}

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



// #include <iostream>
// #include <future>
// #include <atomic>

// std::atomic<bool> cancel_flag(false); // Флаг для отмены выполнения future

// void do_work(std::promise<void>& p) {
//     // Проверяем флаг отмены перед выполнением работы
//     if (cancel_flag) {
//         p.set_exception(std::make_exception_ptr(std::runtime_error("Отмена выполнения")));
//         return;
//     }

//     // Имитируем выполнение работы
//     std::this_thread::sleep_for(std::chrono::seconds(5));

//     // Выполняем обещание, если работа успешно завершена
//     p.set_value();
// }

// int main() {
//     std::promise<void> prom;
//     std::future<void> fut = prom.get_future();

//     // Запускаем выполнение работы
//     std::thread worker(do_work, std::ref(prom));

//     // Пытаемся получить результат future, но не более 3 секунд
//     std::future_status status = fut.wait_for(std::chrono::seconds(3));
//     if (status == std::future_status::timeout) {
//         // Если превышено время ожидания, отменяем выполнение работы
//         cancel_flag = true;
//     }

//     // Ждем завершения работы
//     worker.join();

//     try {
//         // Получаем результат future
//         fut.get();
//         std::cout << "Работа успешно выполнена.\n";
//     } catch (const std::exception& e) {
//         std::cout << "Работа отменена: " << e.what() << "\n";
//     }

//     return 0;
// }


// int main()
// {
//     int sock = 0;
//     struct sockaddr_in serv_addr;

//     if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
//     {
//         throw std::runtime_error("socket");
//     }

//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(PORT);

//     if (inet_pton(PF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
//         std::cerr << "Invalid address/ Address not supported" << std::endl;
//         return 1;
//     }

//     // Connect to server
//     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//         std::cerr << "Connection Failed" << std::endl;
//         return 1;
//     }

//     int epoll_fd = epoll_create1(0);
//     if(epoll_fd == -1)
//     {
//         throw std::runtime_error("epoll_fd");
//     }
    
//     struct epoll_event event;
//     event.events = EPOLLIN;
//     event.data.fd = sock;
//     std::cout << "before before" << std::endl;
//     if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event) == -1)
//     {
//         throw std::runtime_error("epoll_ctl == -1");
//     }
//     std::cout << "before" << std::endl;
//     std::future<void> f1 = std::async([epoll_fd, sock]{epoll_loop(epoll_fd, sock);});
//     std::future<void> f2 = std::async([sock]{handler_write(sock);});
//     f1.get();
//     f2.get();
// }