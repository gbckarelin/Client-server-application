#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iomanip>

class Client
{
public:
    Client(const std::string &name, const std::string &server_ip, int server_port, int interval)
        : name(name), server_ip(server_ip), server_port(server_port), interval(interval) {}

    void start()
    {
        while (true)
        {
            int client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket == -1)
            {
                std::cerr << "Failed to create socket" << std::endl;
                return;
            }

            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port);
            if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
            {
                std::cerr << "Invalid address" << std::endl;
                return;
            }

            if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
            {
                std::cerr << "Connection failed" << std::endl;
                return;
            }

            std::string message = get_current_time() + " \"" + name + "\"";
            send(client_socket, message.c_str(), message.length(), 0);

            close(client_socket);
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
    }

private:
    std::string name;
    std::string server_ip;
    int server_port;
    int interval;

    std::string get_current_time()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        std::tm *tm_info = std::localtime(&now_time);
        std::stringstream ss;
        ss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setw(3) << std::setfill('0') << (ms.count() % 1000);
        return ss.str();
    }
};

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <name> <port> <interval>" << std::endl;
        return 1;
    }
    std::string name = argv[1];
    int port = std::stoi(argv[2]);
    int interval = std::stoi(argv[3]);

    Client client(name, "127.0.0.1", port, interval);
    client.start();
    return 0;
}
