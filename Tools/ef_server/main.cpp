// ef_server — development bridge: exposes JsonApi over TCP (JSON lines, one request per line).
// Used by the Godot project (World.gd, StreamPeerTCP) when the GDExtension is not built.
// Single client, localhost only. Not a shipping component.
//
//   ef_server [--port 7777] [--save-dir ./Saved]
#include "EFWorldCore/JsonApi.h"
#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef SOCKET sock_t;
#  define CLOSESOCK closesocket
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
   typedef int sock_t;
#  define INVALID_SOCKET (-1)
#  define CLOSESOCK close
#endif

static bool sendAll(sock_t s, const std::string& data) {
    size_t off = 0;
    while (off < data.size()) {
        long n = static_cast<long>(send(s, data.data() + off, static_cast<int>(data.size() - off), 0));
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

int main(int argc, char** argv) {
    int port = 7777; std::string saveDir = "./Saved";
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--port") && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--save-dir") && i + 1 < argc) saveDir = argv[++i];
    }
#ifdef _WIN32
    WSADATA wsa; if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { std::fprintf(stderr, "WSAStartup failed\n"); return 1; }
#endif
    sock_t listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) { std::fprintf(stderr, "socket() failed\n"); return 1; }
    int yes = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof yes);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(static_cast<unsigned short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) { std::fprintf(stderr, "bind() failed on port %d\n", port); return 1; }
    if (listen(listener, 1) != 0) { std::fprintf(stderr, "listen() failed\n"); return 1; }
    std::printf("[ef_server] EFWorldCore listening on 127.0.0.1:%d (save dir %s)\n", port, saveDir.c_str());
    std::fflush(stdout);

    ef::JsonApi api(saveDir);   // world persists across client reconnects (editor restarts) within one server run
    for (;;) {
        sock_t client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        std::printf("[ef_server] client connected\n"); std::fflush(stdout);
        std::string buffer; char chunk[65536];
        for (;;) {
            long n = static_cast<long>(recv(client, chunk, sizeof chunk, 0));
            if (n <= 0) break;
            buffer.append(chunk, static_cast<size_t>(n));
            size_t nl;
            while ((nl = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, nl); buffer.erase(0, nl + 1);
                if (line.empty()) continue;
                if (line == "quit") { CLOSESOCK(client); CLOSESOCK(listener); std::printf("[ef_server] quit\n"); return 0; }
                std::string reply = api.handleText(line);
                reply.push_back('\n');
                if (!sendAll(client, reply)) break;
            }
        }
        CLOSESOCK(client);
        std::printf("[ef_server] client disconnected\n"); std::fflush(stdout);
    }
}
