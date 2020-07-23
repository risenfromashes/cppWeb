#ifndef __CW_SESSION_H_
#define __CW_SESSION_H_

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
    virtual void onAborted()  = 0;
    virtual bool onWritable() = 0;
    virtual void onData()     = 0;
    virtual bool shouldEnd()  = 0;
};
} // namespace cW

#endif