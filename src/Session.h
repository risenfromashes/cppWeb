#ifndef __CW_SESSION_H_
#define __CW_SESSION_H_
#include <string_view>
namespace cW {
class ClientSocket;
class Session {
    friend class ClientSocket;
    friend class Server;

  protected:
    enum Type { HTTP, WS };
    ClientSocket* socket;
    Type          type;
    Session(ClientSocket* socket, Type type);
    virtual void onAwakePre()                         = 0;
    virtual void onAwakePost()                        = 0;
    virtual void onAborted()                          = 0;
    virtual void onWritable()                         = 0;
    virtual void onData(const std::string_view& data) = 0;
    virtual bool shouldEnd()                          = 0;
};
} // namespace cW

#endif