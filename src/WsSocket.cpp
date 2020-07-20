#include "WsSocket.h"

namespace cW {

WsSocket::WsSocket(ClientSocket* other)
    : ClientSocket(other), webSocket(new WebSocket(new HttpRequest(receiveBuffer)))
{
    upgraded = true;
}

//[shouldCancel, complete]
std::pair<bool, bool> WsSocket::parseFrame()
{
    WsFrame* frame;
    if (!framePending) {
        if (receiveBuffer.size() < 2) return {false, false};

        // creating new frame
        char* data = receiveBuffer.data();
        frame      = (WsFrame*)malloc(sizeof(WsFrame));

        static_assert(sizeof(WsFrame::header) == 2, "Frame header size is larger than two bytes");

        std::memcpy(&(frame->header), data, 2);

        if (!frame->header.mask) return {true, false};

        unsigned int headerLength = 2;
        if (frame->header.payloadLenShort == 126) {
            headerLength += 2;
            uint16_t length;
            std::memcpy(&length, data + 2, 2);
            reverseByteOrder(&length);
            frame->payloadLength = (uint64_t)length;
        }
        else if (frame->header.payloadLenShort == 127) {
            headerLength += 8;
            uint64_t length;
            std::memcpy(&length, data + 2, 8);
            reverseByteOrder(&length);
            frame->payloadLength = (uint64_t)length;
        }
        else
            frame->payloadLength = (uint64_t)frame->header.payloadLenShort;

        std::memcpy(&(frame->mask[0]), data + headerLength, 4);

        currentFrame = frame;
        framePending = true;
        // removing header part
        receiveBuffer = receiveBuffer.substr(headerLength + 4);
    }
    else
        frame = currentFrame;

    auto payloadLength = frame->payloadLength;
    if (receiveBuffer.size() >= payloadLength) {
        char* data = receiveBuffer.data();
        unMask((uint8_t*)data, payloadLength, frame->mask);
        payloadBuffer.append(data, payloadLength);
        receiveBuffer = receiveBuffer.substr(payloadLength);
        framePending  = false;
        return {false, currentFrame->header.fin == 1};
    }
    return {false, false};
}

// format frames and add them to the queue
void WsSocket::formatFrames(const char* payload, size_t payloadLength, WsOpcode opcode)
{
    WsFrameHeader header;
    size_t        framePayloadSize;
    bool          last;
    if (last = header.fin = (payloadLength <= opts.MaxPayloadLength))
        framePayloadSize = payloadLength;
    else
        framePayloadSize = opts.MaxPayloadLength;
    // server doesn't mask
    header.mask = false;
    // no meaning for these yet
    header.rsv1 = header.rsv2 = header.rsv3 = false;
    // opcode enum values are set accordingly
    header.opcode = (uint8_t)opcode;

    Frame* frame          = (Frame*)malloc(sizeof(Frame));
    frame->opcode         = opcode;
    size_t   headerLength = 2;
    void*    extendedLen;
    uint16_t extendedLenSize = 0;
    if (payloadLength <= 125) { header.payloadLenShort = (uint8_t)payloadLength; }
    else if (payloadLength <= UINT16_MAX) {
        header.payloadLenShort = (uint8_t)126;
        headerLength += 2;
        uint16_t* exlen = (uint16_t*)malloc(extendedLenSize = 2);
        *exlen          = (uint16_t)payloadLength;
        reverseByteOrder(exlen);
        extendedLen = exlen;
    }
    else {
        header.payloadLenShort = (uint8_t)127;
        headerLength += 8;
        uint64_t* exlen = (uint64_t*)malloc(extendedLenSize = 8);
        *exlen          = (uint64_t)payloadLength;
        reverseByteOrder(exlen);
        extendedLen = exlen;
    }
    frame->size = headerLength + framePayloadSize;
    frame->data = (char*)malloc(frame->size);

    std::memcpy(frame->data, &header, 2);
    if (extendedLenSize) {
        std::memcpy(frame->data + 2, extendedLen, extendedLenSize);
        free(extendedLen);
    }
    std::memcpy(frame->data + headerLength, &header, framePayloadSize);

    queuedFrames.push(frame);
    if (!last)
        formatFrames(
            payload + framePayloadSize, payloadLength - framePayloadSize, WsOpcode::Continuation);
}

void WsSocket::cleanUp() { delete webSocket; }

WsSocket::~WsSocket()
{
    delete webSocket;
    freeWsFrame(currentFrame);
    while (!queuedFrames.empty()) {
        delete queuedFrames.front();
        queuedFrames.pop();
    }
}

void WsSocket::loopPreCb()
{
    while (!webSocket->queuedMessages.empty()) {
        auto&& message = webSocket->queuedMessages.front();
        formatFrames(message->payload, message->payloadSize, message->opcode);
        delete message;
        queuedFrames.pop();
    }
}

// receives and writes one message at a time
void WsSocket::onWritable()
{
    if (!queuedFrames.empty()) {
        auto&& frame   = queuedFrames.front();
        int    toWrite = (int)(frame->size - writeOffset);
        writeOffset += write(frame->data + writeOffset, toWrite, queuedFrames.size() == 1);
        assert(writeOffset <= frame->size && "How did write exceed frame size?");
        if (writeOffset == frame->size) {
            if (frame->opcode == WsOpcode::Close) disconnect();
            freeFrame(frame);
            queuedFrames.pop();
            writeOffset = 0;
        }
    }
}
void WsSocket::onData()
{
    auto [shouldClose, complete] = parseFrame();
    if (shouldClose) return disconnect();
    if (complete) {
        webSocket->currentMessage = new WsMessage((WsOpcode)currentFrame->header.opcode,
                                                  payloadBuffer.c_str(),
                                                  currentFrame->payloadLength);
        assert(currentFrame->header.opcode != 0 && "How is the opcode zero here?");
        switch (currentFrame->header.opcode) {
            case 1:
            case 2: server->dispatch(WsEvent::MESSAGE, webSocket); break;
            case 8:
                formatFrames(payloadBuffer.c_str(), payloadBuffer.length(), WsOpcode::Close);
                // actually close the socket after writing close frame
                break;
            case 9:
                formatFrames(payloadBuffer.c_str(), payloadBuffer.size(), WsOpcode::Pong);
                server->dispatch(WsEvent::PING, webSocket);
                break;
            case 10: server->dispatch(WsEvent::PONG, webSocket); break;
            default:
                std::cout << "Unsupported opcode! on socket " << id << " from ip " << ip
                          << std::endl;
                break;
        }
        payloadBuffer.clear();
        freeWsFrame(currentFrame);
        delete webSocket->currentMessage;
        currentFrame              = nullptr;
        webSocket->currentMessage = nullptr;
    }
}

void WsSocket::onAborted()
{
    // unexpected shutdown
    // TODO: add an abort event handler
    // server->dispatch(WsEvent::CLOSE, )
}

}; // namespace cW