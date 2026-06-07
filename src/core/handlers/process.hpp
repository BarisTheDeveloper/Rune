#ifndef RUNE_HANDLERS_PROCESS_HPP
#define RUNE_HANDLERS_PROCESS_HPP

#include <string>

namespace rune::ipc { class Server; }

namespace rune::handlers {

void registerProcessHandlers(rune::ipc::Server& server);

} // namespace rune::handlers

#endif
