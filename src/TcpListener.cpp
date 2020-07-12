#include "TcpListener.h"
#include "Server.h"
namespace cW {

TcpListenerError::TcpListenerError(const char* message, int wsa_error_code)
    : error_code(wsa_error_code),
      std::runtime_error(("WSA ERROR " + std::to_string(wsa_error_code)).c_str())
{
    error_message =
        (std::string(message) + "\nWSA ERROR " + std::to_string(wsa_error_code)).c_str();
}

const char* TcpListenerError::what() const noexcept { return error_message; }

TcpListener::TcpListener(unsigned short port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
        throw TcpListenerError("WSAStartup failed.", WSAGetLastError());

    addrinfo hints, *local_host_addr, *lan_addr;
    char     port_str[16];
    snprintf(port_str, 16, "%d", port);
    // ipv4 only for now
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char* error_message;
    if (error_message = createListenSocket(
            local_host_socket, nullptr, port_str, hints, local_host_addr, "local host")) {
        WSACleanup();
        throw TcpListenerError(error_message, WSAGetLastError());
    }
    gethostname(hostname, 80);
    if (error_message =
            createListenSocket(lan_socket, hostname, port_str, hints, lan_addr, "lan ip")) {
        cleanUp(local_host_addr, local_host_socket);
        WSACleanup();
        throw TcpListenerError(error_message, WSAGetLastError());
    }
};

TcpListener::TcpListener(unsigned short port, Server* server) : TcpListener(port)
{
    this->server = server;
}

void TcpListener::listenOnce(TIMEVAL* timeout)
{
    FD_ZERO(&fds);
    FD_SET(local_host_socket, &fds);
    FD_SET(lan_socket, &fds);
    int activity = select(0, &fds, nullptr, nullptr, timeout);
    if (activity > 0) {
        if (FD_ISSET(local_host_socket, &fds)) acceptSocket(local_host_socket);
        if (FD_ISSET(lan_socket, &fds)) acceptSocket(lan_socket);
    }
    else if (activity == SOCKET_ERROR) {
        close();
        throw TcpListenerError("Error selecting listen sockets.", WSAGetLastError());
    }
}

void TcpListener::close()
{
    closesocket(local_host_socket);
    closesocket(lan_socket);
    WSACleanup();
}
TcpListener::~TcpListener() { close(); }

inline void TcpListener::socketHandler(SOCKET Socket, std::string ip)
{
    if (server) { server->handleSocket(Socket, ip); }
    else if (handler)
        handler(Socket, ip);
    else {
        char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n\r\n<html>Hello</html>";
        int   bytes_wrote = send(Socket, response, (int)strlen(response), 0);
        if (bytes_wrote < 0) {
            std::cout << "Write failed with error " << WSAGetLastError() << std::endl;
        }
        else
            std::cout << "Wrote " << bytes_wrote << " bytes successfully." << std::endl;
        if (shutdown(Socket, SD_SEND) == SOCKET_ERROR) {
            std::cout << "Failed to close client socket. Error " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "Closed client socket successfully." << std::endl;
        }
    }
}

inline void TcpListener::acceptSocket(const SOCKET& socket_handle)
{
    SOCKET      Socket = INVALID_SOCKET;
    sockaddr_in clientaddr;
    int         clientaddrlen = sizeof(clientaddr);
    char        ip[16];
    if ((Socket = accept(socket_handle, (sockaddr*)&clientaddr, &clientaddrlen)) ==
        INVALID_SOCKET) {
        std::cout << "Accept failed with error " << WSAGetLastError() << std::endl;
    }
    else {
        inet_ntop(AF_INET, &clientaddr.sin_addr, ip, 16);
        std::cout << "Accepted socket from ip: " << ip << std::endl;
        socketHandler(Socket, ip);
    }
}

char* TcpListener::createListenSocket(SOCKET&     socket_handle,
                                      const char* host,
                                      const char* port,
                                      addrinfo&   hints,
                                      addrinfo*&  addr_info,
                                      const char* name)
{
    char* error_message = (char*)malloc(sizeof(char) * 80);
    if (getaddrinfo(host, port, &hints, &addr_info)) {
        snprintf(error_message, 80, "Couldn't get address of %s.", name);
        throw TcpListenerError(error_message, WSAGetLastError());
    }
    if ((socket_handle =
             socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)) ==
        INVALID_SOCKET) {
        freeaddrinfo(addr_info);
        snprintf(error_message, 80, "Creation of %s socket failed.", name);
    }
    if ((bind(socket_handle, addr_info->ai_addr, (int)addr_info->ai_addrlen))) {
        freeaddrinfo(addr_info);
        closesocket(socket_handle);
        snprintf(error_message, 80, "Binding of %s socket failed.", name);
    }
    if (listen(socket_handle, SOMAXCONN) == SOCKET_ERROR) {
        freeaddrinfo(addr_info);
        closesocket(socket_handle);
        snprintf(error_message, 80, "Listening on %s socket failed.", name);
    }
    return nullptr;
}

inline void TcpListener::cleanUp(addrinfo*& addr_info, SOCKET& socket_handle)
{
    freeaddrinfo(addr_info);
    closesocket(socket_handle);
}

}; // namespace cW