#include "server.hpp"

#define PORT 7300

int main()
{
    Server *s = new Server(PORT, BUFFER_SIZE);
    
    for(;;)
    {   
        std::atomic<bool> stop(false);
        s->create_socket();
        s->bind_to_port();
        if(s->listen_client() < 0)
        {
            std::cout << "xnj nj yt nj" << std::endl;
        }
        std::cout << "1" << std::endl;
        std::future<void> f1 = std::async([s, &stop]{s->handle_events(stop);});
        f1.get();
    }      
    delete s;
}