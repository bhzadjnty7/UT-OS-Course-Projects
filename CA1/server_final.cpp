#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// data stractures
struct ClientInfo {
    bool signUpDone;
    bool inTeam;
    std::string username;
    std::string role;
    int teamId;
};

struct Team {
    int teamId;
    int sockCoder;
    int sockNav;
    std::string coderName;
    std::string navName;
    int score;

    bool waitingForReady;   // accept yes only frome coder
    bool compStarted;     
    bool compFinished;      //when they answer all Q or the time is finished
    int currentProblemIndex;
    std::time_t problemStartTime;
};

struct Problem {
    int id;
    std::string title;
    std::string keyword; // "add_numbers", "reverse_string", "is_palindrome"
    int difficulty;      // 1, 3, 5
};

int serverSock;          // TCP
int udpSock;             // UDP
sockaddr_in broadcastAddr;

std::vector<int> clientSockets;
std::map<int, ClientInfo> clients;
std::map<std::string, bool> usedUsernames;
std::queue<int> coderQueue;
std::queue<int> navigatorQueue;
std::vector<Team> teams;
int nextTeamId = 1;

std::vector<Problem> problemSet = {
    {1, "Add two numbers",   "add_numbers",    1},
    {2, "Reverse a string",  "reverse_string", 3},
    {3, "Check palindrome",  "is_palindrome",  5}
};

int problemDurationSec = 60;

bool competitionStarted = false;  // not started yet
bool competitionFinished = false;

// helping function
void sendToClient(int sock, const std::string &msg) {
    send(sock, msg.c_str(), msg.size(), 0);     //send msg to cliend by sock
}

void broadcastMessage(const std::string &msg) {
    sendto(udpSock, msg.c_str(), msg.size(), 0,
           reinterpret_cast<sockaddr*>(&broadcastAddr), sizeof(broadcastAddr));
}

bool initUDPBroadcast(int port) {
    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        std::cerr << "Error: could not create UDP socket.\n";
        return false;
    }
    int opt = 1;
    if (setsockopt(udpSock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: setsockopt(SO_BROADCAST) failed.\n";
        close(udpSock);
        return false;
    }
    std::memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    return true;
}

Team* findTeamById(int tid) {
    for (auto &t : teams) {
        if (t.teamId == tid)
            return &t;
    }
    return nullptr;
}

//sending problems
void sendProblemSetToClient(int sock) {
    std::string ps = "Problem Set:\n";
    for (const auto &p : problemSet) {
        ps += "Q" + std::to_string(p.id) + ": " + p.title + " (diff=" + std::to_string(p.difficulty) + ")\n";
    }
    sendToClient(sock, ps);
}

// managing teams
void formTeam(int coderSock, int navSock) {
    ClientInfo &cCoder = clients[coderSock];
    ClientInfo &cNav   = clients[navSock];

    Team t;
    t.teamId = nextTeamId++;
    t.sockCoder = coderSock;
    t.sockNav   = navSock;
    t.coderName = cCoder.username;
    t.navName   = cNav.username;
    t.score = 0;
    t.waitingForReady = true;
    t.compStarted = false;
    t.compFinished = false;
    t.currentProblemIndex = 0;
    t.problemStartTime = 0;

    teams.push_back(t);

    cCoder.inTeam = true;
    cNav.inTeam = true;
    cCoder.teamId = t.teamId;
    cNav.teamId = t.teamId;

    // show in server
    std::cout << "[Server] Team FORMED: " << cCoder.username << " & " << cNav.username << "\n";

    // send message to teamates
    sendToClient(coderSock, "TEAM FORMED: " + cCoder.username + " & " + cNav.username + "\n");
    sendToClient(navSock,   "TEAM FORMED: " + cCoder.username + " & " + cNav.username + "\n");

    // send problems to both
    sendProblemSetToClient(coderSock);
    sendProblemSetToClient(navSock);

    sendToClient(coderSock, "READY?\n");
    sendToClient(navSock, "READY?\n");
}

// try for pairing
void tryPairing(int sock, const std::string &role) {
    if (role == "coder") {
        if (!navigatorQueue.empty()) {
            int nSock = navigatorQueue.front();
            navigatorQueue.pop();
            formTeam(sock, nSock);
        } else {
            coderQueue.push(sock);
        }
    } else { // role == "navigator"
        if (!coderQueue.empty()) {
            int cSock = coderQueue.front();
            coderQueue.pop();
            formTeam(cSock, sock);
        } else {
            navigatorQueue.push(sock);
        }
    }
}

// start compettition
void startTeamProblem(Team &team) {
    if (team.currentProblemIndex >= (int)problemSet.size()) {
        team.compFinished = true;
        sendToClient(team.sockCoder, "[Server] All problems done.\n");
        sendToClient(team.sockNav, "[Server] All problems done.\n");
        std::cout << "[Server] Team " << team.teamId << " finished all problems.\n";
        return;
    }
    team.problemStartTime = std::time(nullptr);
    Problem &p = problemSet[team.currentProblemIndex];
    std::string msg = "START Problem #" + std::to_string(p.id) + ": " + p.title +
                      " (diff=" + std::to_string(p.difficulty) + ", " +
                      std::to_string(problemDurationSec) + "s)\n";
    sendToClient(team.sockCoder, msg);
    sendToClient(team.sockNav, msg);
    std::cout << "[Server] Team " << team.teamId << " started Problem #" 
              << p.id << " (" << p.title << ").\n";
}

// checking each teams time
void checkTeamDeadline(Team &team) {
    if (!team.compStarted || team.compFinished)
        return;
    if (team.currentProblemIndex >= (int)problemSet.size())
        return;

    std::time_t now = std::time(nullptr);
    if (now > team.problemStartTime + problemDurationSec) {
        Problem &p = problemSet[team.currentProblemIndex];
        std::string msg = "TIME IS UP for problem #" + std::to_string(p.id) + "\n";
        sendToClient(team.sockCoder, msg);
        sendToClient(team.sockNav, msg);
        std::cout << "[Server] Team " << team.teamId << ": Time is up for Problem #" 
                  << p.id << ". Moving to next problem.\n";

        team.currentProblemIndex++;
        if (team.currentProblemIndex >= (int)problemSet.size()) {
            team.compFinished = true;
            sendToClient(team.sockCoder, "[Server] All problems done.\n");
            sendToClient(team.sockNav, "[Server] All problems done.\n");
            std::cout << "[Server] Team " << team.teamId << " finished all problems.\n";
        } else {
            startTeamProblem(team);
        }
    }
}

// ranking
void finishCompetition() {
    broadcastMessage("All teams finished! Final scores:\n");
    std::vector<Team> sorted = teams;
    std::sort(sorted.begin(), sorted.end(),
        [](const Team &a, const Team &b){ return b.score < a.score; });
    std::string res;
    for (auto &tm : sorted) {
        res += "Team " + std::to_string(tm.teamId) + " (" + tm.coderName + "+" + tm.navName + "): " + std::to_string(tm.score) + "\n";
    }
    broadcastMessage(res);
    std::cout << "[Server] Competition finished. Results broadcasted.\n";
}

// check answers
bool checkPythonSignature(const std::string &keyword, const std::string &code) {
    if (keyword == "add_numbers")
        return (code.find("def add_numbers(a, b):") != std::string::npos);
    if (keyword == "reverse_string")
        return (code.find("def reverse_string(s):") != std::string::npos);
    if (keyword == "is_palindrome")
        return (code.find("def is_palindrome(s):") != std::string::npos);
    return false;
}

void evaluateAndScore(Team &team, const Problem &pb, const std::string &code) {
    if (!checkPythonSignature(pb.keyword, code)) {
        sendToClient(team.sockNav, "Failed (Signature mismatch)\n");
        broadcastMessage("TEAM " + team.coderName + "+" + team.navName +
                         " failed (signature) Problem #" + std::to_string(pb.id) + "\n");
        std::cout << "[Server] Team " << team.teamId << " failed Problem #" << pb.id << " (Signature mismatch).\n";
        return;
    }
    if (code.find("OK") == std::string::npos) {
        sendToClient(team.sockNav, "FAILED (no 'OK')\n");
        broadcastMessage("TEAM " + team.coderName + "+" + team.navName +
                         " failed Problem #" + std::to_string(pb.id) + "\n");
        std::cout << "[Server] Team " << team.teamId << " failed Problem #" << pb.id << " (no 'OK').\n";
        return;
    }
    int baseScore = pb.difficulty;
    std::time_t now = std::time(nullptr);
    int elapsed = (int)(now - team.problemStartTime);
    int halfTime = problemDurationSec / 2;
    int threeFour = (3 * problemDurationSec) / 4;
    float bonusFactor = 0.0f;
    if (elapsed <= halfTime)
        bonusFactor = 0.5f;
    else if (elapsed <= threeFour)
        bonusFactor = 0.2f;
    int bonus = (int)(baseScore * bonusFactor);
    int totalPoints = baseScore + bonus;

    team.score += totalPoints;

    std::string resultMsg = team.coderName + " & " + team.navName + " passed Question #" +
                            std::to_string(pb.id) + " and got " + std::to_string(totalPoints) + "\n";
    sendToClient(team.sockNav, "PASSED. Gained " + std::to_string(totalPoints) +
                              " => total score=" + std::to_string(team.score) + "\n");
    broadcastMessage("TEAM " + team.coderName + "+" + team.navName +
                     " solved Problem #" + std::to_string(pb.id) +
                     " (+" + std::to_string(totalPoints) + ")\n");
    std::cout << "[Server] " << resultMsg;
}

// solutions
void handleSolution(int sock, const std::string &msg) {
    // format: "SOLVE <problemId> <code>"
    size_t sp1 = msg.find(' ');
    if (sp1 == std::string::npos) return;
    size_t sp2 = msg.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return;

    std::string pidStr = msg.substr(sp1 + 1, sp2 - (sp1 + 1));
    std::string code   = msg.substr(sp2 + 1);

    auto &ci = clients[sock];
    if (ci.teamId <= 0) {
        sendToClient(sock, "No team assigned!\n");
        return;
    }
    Team* team = findTeamById(ci.teamId);
    if (!team) {
        sendToClient(sock, "Team not found.\n");
        return;
    }
    if (!team->compStarted || team->compFinished) {
        sendToClient(sock, "Competition not active for your team.\n");
        return;
    }
    std::time_t now = std::time(nullptr);
    if (now > team->problemStartTime + problemDurationSec) {
        sendToClient(sock, "Time is up for this problem.\n");
        return;
    }
    Problem &pb = problemSet[team->currentProblemIndex];
    int probIdGiven = std::stoi(pidStr);
    if (pb.id != probIdGiven) {
        sendToClient(sock, "Wrong problem ID.\n");
        return;
    }
    evaluateAndScore(*team, pb, code);

    // next problems
    team->currentProblemIndex++;
    if (team->currentProblemIndex >= (int)problemSet.size()) {
        team->compFinished = true;
        sendToClient(team->sockCoder, "[Server] All problems done.\n");
        sendToClient(team->sockNav, "[Server] All problems done.\n");
        std::cout << "[Server] Team " << team->teamId << " finished all problems.\n";
    } else {
        startTeamProblem(*team);
    }
}

// ready? yes
void processReadyResponse(int sock, const std::string &msg) {
    if (msg != "yes")
        return;
    auto &ci = clients[sock];
    if (ci.teamId <= 0)
        return;
    Team* team = findTeamById(ci.teamId);
    if (team && team->waitingForReady && !team->compStarted) {
        team->waitingForReady = false;
        team->compStarted = true;
        team->currentProblemIndex = 0;
        startTeamProblem(*team);
        std::cout << "[Server] Team " << team->teamId << " confirmed ready. Competition started for this team.\n";
    }
}

// main
int main(int argc, char* argv[]){
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <tcp_port> <udp_port>\n";
        return 1;
    }
    int tcpPort = std::atoi(argv[1]);
    int udpPort = std::atoi(argv[2]);

    // TCP socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "Error: socket creation.\n";
        return 1;
    }
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in saddr;
    std::memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port   = htons(tcpPort);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&saddr, sizeof(saddr)) < 0) {
        std::cerr << "Error: bind.\n";
        return 1;
    }
    if (listen(serverSock, 5) < 0) {
        std::cerr << "Error: listen.\n";
        return 1;
    }
    std::cout << "[Server] Listening on TCP port " << tcpPort << "\n";

    // UDP port
    if (!initUDPBroadcast(udpPort)) {
        std::cerr << "Error: initUDPBroadcast.\n";
        close(serverSock);
        return 1;
    }
    std::cout << "[Server] Broadcasting on UDP port " << udpPort << "\n";


    while (!competitionFinished) {
        // check each team time
        for (auto &team : teams) {
            if (team.compStarted && !team.compFinished) {
                checkTeamDeadline(team);
            }
        }

        bool allDone = true;
        for (auto &team : teams) {
            if (!team.compFinished) {
                allDone = false;
                break;
            }
        }
        if (allDone && !teams.empty()) {
            finishCompetition();
            break;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);
        int maxfd = serverSock;
        for (int c : clientSockets) {
            FD_SET(c, &readfds);
            if (c > maxfd)
                maxfd = c;
        }
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (activity <= 0)
            continue;

        // new connection
        if (FD_ISSET(serverSock, &readfds)) {
            sockaddr_in cAddr;
            socklen_t cLen = sizeof(cAddr);
            int newSock = accept(serverSock, (sockaddr*)&cAddr, &cLen);
            if (newSock >= 0) {
                clientSockets.push_back(newSock);
                ClientInfo ci;
                ci.signUpDone = false;
                ci.inTeam = false;
                ci.teamId = 0;
                clients[newSock] = ci;
                std::cout << "[Server] New client sock=" << newSock << "\n";
            }
        }

        // recive messages from clients
        for (size_t i = 0; i < clientSockets.size(); i++) {
            int csock = clientSockets[i];
            if (FD_ISSET(csock, &readfds)) {
                char buffer[512];
                std::memset(buffer, 0, sizeof(buffer));
                ssize_t rec = recv(csock, buffer, sizeof(buffer), 0);
                if (rec <= 0) {
                    std::string oldUser = clients[csock].username;
                    int oldTeam = clients[csock].teamId;
                    clients.erase(csock);
                    close(csock);
                    clientSockets.erase(clientSockets.begin() + i);
                    i--;
                    if (!oldUser.empty())
                        usedUsernames.erase(oldUser);
                    if (oldTeam > 0) {
                        Team* t = findTeamById(oldTeam);
                        if (t) {
                            int otherSock = (t->sockCoder == csock) ? t->sockNav : t->sockCoder;
                            if (clients.find(otherSock) != clients.end()) {
                                sendToClient(otherSock, "Your teammate (" + oldUser + ") disconnected.\n");
                            }
                        }
                    }
                } else {
                    std::string msg(buffer);
                    ClientInfo &ci = clients[csock];
                    if (!ci.signUpDone) {
                        size_t sp = msg.find(' ');
                        if (sp == std::string::npos) {
                            sendToClient(csock, "Format: <username> <role>\n");
                            continue;
                        }
                        std::string uname = msg.substr(0, sp);
                        std::string r = msg.substr(sp + 1);
                        while (!uname.empty() && (uname.back() == '\n' || uname.back() == '\r'))
                            uname.pop_back();
                        while (!r.empty() && (r.back() == '\n' || r.back() == '\r'))
                            r.pop_back();
                        if (usedUsernames.find(uname) != usedUsernames.end()) {
                            sendToClient(csock, "DUPLICATE\n");
                            continue;
                        }
                        usedUsernames[uname] = true;
                        ci.username = uname;
                        ci.role = r;
                        ci.signUpDone = true;
                        sendToClient(csock, "OK\n");
                        std::cout << "[Server] " << uname << " signed up as a " << r << "\n";
                        if (r != "coder" && r != "navigator") {
                            sendToClient(csock, "Role must be 'coder' or 'navigator'.\n");
                        } else {
                            // send problems to clients
                            sendProblemSetToClient(csock);
                            tryPairing(csock, r);
                        }
                    } else {
                        if (ci.role == "coder" && msg == "yes") {
                            // if and only if coder said yes
                            Team* t = findTeamById(ci.teamId);
                            if (t && t->waitingForReady && !t->compStarted) {
                                t->waitingForReady = false;
                                t->compStarted = true;
                                t->currentProblemIndex = 0;
                                t->problemStartTime = std::time(nullptr);
                                startTeamProblem(*t);
                                std::cout << "[Server] Team " << t->teamId << " confirmed ready. Competition started for this team.\n";
                            }
                        } else if (ci.role == "navigator" && msg.rfind("SOLVE ", 0) == 0) {
                            handleSolution(csock, msg);
                        } else {
                            sendToClient(csock, "[Echo] " + msg);
                        }
                    }
                }
            }
        }
    }

    for (int s : clientSockets) {
        close(s);
    }
    close(serverSock);
    close(udpSock);
    return 0;
}
