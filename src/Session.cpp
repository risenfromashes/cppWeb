#include "Session.h"

namespace cW {
Session::Session(ClientSocket* socket, Type type) : socket(socket), type(type) {}
} // namespace cW