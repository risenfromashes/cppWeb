#ifndef __CW_WEB_SOCKET_FRAME_H_
#define __CW_WEB_SOCKET_FRAME_H_

#include <queue>
#include "Session.h"
#include "WebSocket.h"

// 0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-------+-+-------------+-------------------------------+
//  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
//  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
//  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
//  | |1|2|3|       |K|             |                               |
//  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
//  |     Extended payload length continued, if payload len == 127  |
//  + - - - - - - - - - - - - - - - +-------------------------------+
//  |                               |Masking-key, if MASK set to 1  |
//  +-------------------------------+-------------------------------+
//  | Masking-key (continued)       |          Payload Data         |
//  +-------------------------------- - - - - - - - - - - - - - - - +
//  :                     Payload Data continued ...                :
//  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
//  |                     Payload Data continued ...                |
//  +---------------------------------------------------------------+

namespace cW {

struct WebSocketOpts {
    size_t MaxPayloadLength = 1024 * 1024;
};
class WebSocketSession : Session {

    friend class Poll;
    friend class ClientSocket;
    friend class Server;

    // formated binary frame
    struct Frame {
        WsOpcode opcode;
        char*    data;
        size_t   size;
    };
    struct WsFrameHeader {
        uint8_t opcode : 4;
        bool    rsv3 : 1;
        bool    rsv2 : 1;
        bool    rsv1 : 1;
        bool    fin : 1;
        uint8_t payloadLenShort : 7;
        bool    mask : 1;
    };
    struct WsFrame {
        WsFrameHeader header;
        size_t        payloadLength;
        uint8_t       mask[4];
    };
    static inline void freeWsFrame(WsFrame* frame);
    static inline void freeFrame(Frame* frame);

    std::queue<Frame*> queuedFrames;
    // continuation buffer for current frame
    std::string payloadBuffer;
    WsFrame*    currentFrame;

    WebSocket* webSocket;

    bool framePending    = false;
    bool fragmentPending = false;

    WebSocketOpts opts;

    size_t      writeOffset = 0;
    std::string requestHeader;

    WebSocketSession(ClientSocket* socket);

    //[shouldCancel, complete]
    std::pair<bool, bool> parseFrame();

    static inline void unMask(uint8_t* payload, size_t payloadLength, uint8_t (&mask)[4]);

    // format frames and add them to the queue
    void formatFrames(const char* payload, size_t payloadLength, WsOpcode opcode);

    void cleanUp();

    ~WebSocketSession();
    // receives and writes one message at a time

    void onAborted() override;
    bool onWritable() override;
    void onData() override;
    bool shouldEnd() override;
};

void WebSocketSession::unMask(uint8_t* payload, size_t payloadLength, uint8_t (&mask)[4])
{
    for (size_t i = 0; i < payloadLength; i++)
        payload[i] ^= mask[i % 4];
}

void WebSocketSession::freeWsFrame(WsFrame* frame)
{
    if (frame) { free(frame); };
}
void WebSocketSession::freeFrame(Frame* frame)
{
    if (frame) {
        free(frame->data);
        free(frame);
    }
}

} // namespace cW

#endif