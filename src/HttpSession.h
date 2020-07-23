#ifndef __CW_HTTP_SOCKET_H_
#define __CW_HTTP_SOCKET_H_

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Session.h"

namespace cW {

class ClientSocket;

struct HttpSession : Session {
    friend class Poll;
    friend class ClientSocket;
    friend class Server;

    HttpResponse* response      = nullptr;
    HttpRequest*  request       = nullptr;
    bool          wroteHeader   = false;
    size_t        writeOffset   = 0;
    bool          writing       = false;
    bool          sessionEnd    = false;
    bool          dispatched    = false;
    bool          doneWriting   = false;
    bool          doneReceiving = false;
    // request header need to last the life time of the session
    std::string requestHeader;

    HttpSession(ClientSocket* socket);
    void dispatch();
    bool badRequest();
    void onAborted() override;
    void onData() override;
    bool onWritable() override;
    bool shouldEnd() override;
    ~HttpSession();
};

}; // namespace cW

#endif