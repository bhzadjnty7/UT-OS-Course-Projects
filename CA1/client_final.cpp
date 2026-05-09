#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <server_ip> <tcp_port> <udp_listen_port> <role>\n"
                  << "Example: ./client_ready_final 127.0.0.1 9000 20000 navigator\n";
        return 1;
    }

    std::string serverIP = argv[1];
    int tcpPort = std::atoi(argv[2]);
    int udpPort = std::atoi(argv[3]);
    std::string role = argv[4];

    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);

    // --- TCP سوکت
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error: cannot create TCP socket.\n";
        return 1;
    }
    sockaddr_in srv;
    std::memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(tcpPort);
    inet_pton(AF_INET, serverIP.c_str(), &srv.sin_addr);

    if (connect(sock, (sockaddr*)&srv, sizeof(srv)) < 0) {
        std::cerr << "Connect failed.\n";
        close(sock);
        return 1;
    }
    std::cout << "[Client] Connected to " << serverIP << ":" << tcpPort << "\n";

    // ارسال ثبت نام: "<username> <role>"
    {
        std::string reg = username + " " + role;
        send(sock, reg.c_str(), reg.size(), 0);
    }

    // --- UDP سوکت برای دریافت Broadcast
    int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        std::cerr << "Error: cannot create UDP socket.\n";
        close(sock);
        return 1;
    }
    sockaddr_in uAddr;
    std::memset(&uAddr, 0, sizeof(uAddr));
    uAddr.sin_family = AF_INET;
    uAddr.sin_port = htons(udpPort);
    uAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(udpSock, (sockaddr*)&uAddr, sizeof(uAddr)) < 0) {
        std::cerr << "Error: bind UDP failed.\n";
        close(sock);
        close(udpSock);
        return 1;
    }
    std::cout << "[Client] UDP bound on port " << udpPort << "\n";

    bool running = true;
    std::string teammateName;

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        int maxfd = sock;
        FD_SET(udpSock, &readfds);
        if (udpSock > maxfd) maxfd = udpSock;
        FD_SET(STDIN_FILENO, &readfds);
        if (STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            std::cerr << "select error.\n";
            break;
        }

        // دریافت پیام از سرور (TCP)
        if (FD_ISSET(sock, &readfds)) {
            char buffer[512];
            std::memset(buffer, 0, sizeof(buffer));
            ssize_t rec = recv(sock, buffer, sizeof(buffer), 0);
            if (rec <= 0) {
                std::cerr << "[Client] Server disconnected.\n";
                running = false;
            } else {
                std::string data(buffer);
                std::cout << "[Server TCP]: " << data << "\n";
                if (data.rfind("TEAM ", 0) == 0) {
                    teammateName = data.substr(5);
                    while (!teammateName.empty() &&
                           (teammateName.back()=='\n' || teammateName.back()=='\r'))
                        teammateName.pop_back();
                    std::cout << "[Client] Your teammate is " << teammateName << "\n";
                }
                // پیام READY? نیز چاپ می‌شود.
            }
        }

        // دریافت Broadcast (UDP)
        if (FD_ISSET(udpSock, &readfds)) {
            char bbuf[512];
            std::memset(bbuf, 0, sizeof(bbuf));
            sockaddr_in src;
            socklen_t srclen = sizeof(src);
            ssize_t br = recvfrom(udpSock, bbuf, sizeof(bbuf), 0, (sockaddr*)&src, &srclen);
            if (br > 0) {
                std::string bcMsg(bbuf);
                std::cout << "[Broadcast]: " << bcMsg << "\n";
            }
        }

        // ورودی کاربر (stdin)
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string userInput;
            if (!std::getline(std::cin, userInput)) {
                running = false;
            } else {
                if (userInput == "quit") {
                    running = false;
                } else {
                    // ارسال هر ورودی به سرور؛ مانند "yes" برای شروع مسابقه یا "SOLVE ..." برای پاسخ
                    send(sock, userInput.c_str(), userInput.size(), 0);
                }
            }
        }
    }

    close(sock);
    close(udpSock);
    return 0;
}
