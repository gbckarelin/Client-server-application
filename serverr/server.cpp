#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <mutex>

class ConnectionHandler
{
public:
    ConnectionHandler(int client_socket) : client_socket(client_socket) {}

    void operator()()
    {
        char buffer[1024];
        int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            std::lock_guard<std::mutex> lock(file_mutex);
            std::ofstream log_file("log.txt", std::ios::app);
            if (log_file.is_open())
            {
                log_file << buffer << std::endl;
            }
        }
        close(client_socket);
    }

private:
    int client_socket;
    static std::mutex file_mutex;
};

std::mutex ConnectionHandler::file_mutex;

class Server
{
public:
    Server(int port) : port(port) {}

    void start()
    {
        std::ofstream logfile("server_log.txt", std::ios::app);
        int server_socket = createSocket(logfile);
        setSocketOptions(server_socket, logfile);
        bindSocket(server_socket, logfile);
        listenForConnections(server_socket, logfile);
    }

private:
    int port;

    int createSocket(std::ofstream &logfile)
    {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1)
        {
            logMessage(logfile, "Socket creation failed");
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        return server_socket;
    }

    void setSocketOptions(int server_socket, std::ofstream &logfile)
    {
        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        {
            logMessage(logfile, "Setsockopt failed");
            perror("Setsockopt failed");
            exit(EXIT_FAILURE);
        }
    }

    void bindSocket(int server_socket, std::ofstream &logfile)
    {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            logMessage(logfile, "Bind failed");
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }
    }

    void listenForConnections(int server_socket, std::ofstream &logfile)
    {
        if (listen(server_socket, 5) == -1)
        {
            logMessage(logfile, "Listen failed");
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }
        logMessage(logfile, "Server started, waiting for connections...");

        std::vector<std::thread> threads;
        while (true)
        {
            int client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket == -1)
            {
                logMessage(logfile, "Accept failed");
                perror("Accept failed");
                continue;
            }
            threads.emplace_back(ConnectionHandler(client_socket));
        }

        for (auto &t : threads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        close(server_socket);
    }

    void logMessage(std::ofstream &logfile, const std::string &message)
    {
        if (logfile.is_open())
        {
            logfile << message << std::endl;
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]);
    Server server(port);
    server.start();
    return 0;
}
