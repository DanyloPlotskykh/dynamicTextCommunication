#include "client.hpp"

const std::string ADDR = "127.0.0.1";
const int PORT = 7300;

// Why it is not part of Client?
void schedular(Client *cl, std::atomic<bool> &stop, std::atomic<bool> &port_is_changed)
{
    for(size_t i = 0; i < 4500; ++i)
    {
        if(port_is_changed) // cv may help here condition_variable
        {
            std::cout << "HAS BEEN CHANGED" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "tryin' to change port ... " << std::endl;
    int port;

    for(;;)
    {
        port = cl->generate_random_port();
        if(cl->isPortAvailable(port)) { break;}
    }

    std::string message = "newport-" + std::to_string(port);
    send(cl->m_sock, message.c_str(), strlen(message.c_str()), 0);
    stop = true; //Strange logic
}

int main()
{
    // Never use smart pointers
    Client *cl = new Client(ADDR, PORT); // Why it is created on heap?
    for(;;)
    {
        // Why we need that sleep here?
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::atomic<bool> stop(false);
        std::atomic<bool> is_change(false);
        cl->create_socket();
        cl->bind_to_server();

        if(!cl->connect_to_server())
        {
            break;
        }

        // Give better naming to variables
        // What is f1, t1?
        // Do you create threads constantly?
        std::future<void> f1 = std::async([cl, &stop, &is_change]{cl->handle_events(stop, is_change);});
        std::thread thread_write([cl, &stop, &is_change]{cl->handle_write(stop, is_change);});
        std::thread t1([cl, &stop, &is_change]{schedular(cl, stop, is_change);});
        f1.get();
        thread_write.detach();
        t1.detach();
    }
    delete cl;
}