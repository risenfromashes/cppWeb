#ifndef __CW_HTTP_SOCKET_H_
#define __CW_HTTP_SOCKET_H_

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Session.h"

namespace cW {

class ClientSocket;

class HttpSession : Session {
    friend class Poll;
    friend class ClientSocket;
    friend class Server;

    HttpResponse* response      = nullptr;
    HttpRequest*  request       = nullptr;
    bool          wroteHeader   = false;
    size_t        writeOffset   = 0;
    bool          doneWriting   = false;
    bool          doneReceiving = true; // if no data is available, this is the default
    bool          dispatched    = false;
    bool          hasHandler;

    HttpSession(ClientSocket* socket, const std::string_view& requestHeader);
    void dispatch(const std::string_view& requestHeader);
    void badRequest();
    void onAwakePre() override;
    void onAwakePost() override;
    void onAborted() override;
    void onData(const std::string_view& recvBuf) override;
    void onWritable() override;
    bool shouldEnd() override;
    ~HttpSession();
};

}; // namespace cW

#endif