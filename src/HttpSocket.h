#ifndef __CW_HTTP_SOCKET_H_
#define __CW_HTTP_SOCKET_H_

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Socket.h"
#include "Server.h"

namespace cW {

class HttpSocket : public Socket {
    friend class SocketSet;

  private:
    HttpResponse* response;
    HttpRequest*  request;
    bool          wroteHeader = false;

    size_t writeOffset = 0;
    // constructs the socket from parent and delete it afterwards
    HttpSocket(Socket* socket);

  public:
    void onAwake() override;
    void onData() override;
    void onWritable() override;
    bool shouldUpgrade();

    ~HttpSocket();
};

}; // namespace cW

#endif