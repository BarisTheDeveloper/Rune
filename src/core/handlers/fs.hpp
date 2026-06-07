#ifndef RUNE_HANDLERS_FS_HPP
#define RUNE_HANDLERS_FS_HPP

#include <string>

namespace rune::ipc { class Server; }

namespace rune::handlers {

void registerFsHandlers(rune::ipc::Server& server, const std::string& appDir);

} // namespace rune::handlers

#endif
