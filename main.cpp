#include "TCPHandler.h"
#include <iomanip>

int main(int argc, char * argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cout << "Usage: port\n";
            return 1;
        }

        BlockResolver resolver;
        
        std::thread resolverThread([&resolver]() 
        {
            resolver.Resolve();
        });

        tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));

        boost::asio::io_context io_context;
        Server myserv(endpoint, io_context, resolver);
        io_context.run();
    
        resolverThread.join();   
    }

    catch (std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    

    return 0;
}
