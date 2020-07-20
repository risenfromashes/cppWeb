#ifndef __CW_HTTP_SOCKET_H_
#define __CW_HTTP_SOCKET_H_

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "ClientSocket.h"

namespace cW {

struct HttpSocket : public ClientSocket {
    friend class Server;
    friend class Poll;

  private:
    HttpResponse* response;
    HttpRequest*  request;
    bool          wroteHeader = false;
    // request header need to last for the life time of the connection
    std::string requestHeader;
    // these are signed to prevent large numbers on  unsigned substraction
    int64_t requestContentLength = -1;
    int64_t responseHeaderLength = 0;
    int64_t writeOffset          = 0;
    bool    writing              = false;
    HttpSocket(ClientSocket* socket);
    void dispatch();
    void badRequest();

  public:
    void loopPreCb() override;
    void loopPostCb() override;
    void onData() override;
    void onWritable() override;
    ~HttpSocket();
};

}; // namespace cW

#endif