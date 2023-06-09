#ifndef DBVPN_DBVPNCLIENT_H
#define DBVPN_DBVPNCLIENT_H
#define IP_SIZE 128
#define IF_NAME_SIZE 16
#define BUFF_SIZE 4096
#define EPOLL_SIZE 16
#define MOD_SRC 1
#define MOD_DST 0
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <tins/tins.h>
#include <iostream>
#include <set>
#include <ctime>

// Определение класса VpnServer
class VpnServer
{
private:
    // Объявление приватных функций и типов
    void get_local_ip(const char *ifName);
    void create_tun(const char *tunAddress);
    typedef std::pair<Tins::IPv4Address, uint16_t> IpPort;
    typedef std::map<uint16_t, IpPort> PortToAddr;
    typedef std::map<IpPort, uint16_t> AddrToPort;
    typedef std::map<IpPort, struct sockaddr_in> IpPortToSockaddr;
    typedef struct Port_t
    {
        uint16_t port;
        bool used;
        time_t lastTime;
        Port_t(){}
        Port_t(int port_)
        {
            port = port_;
            used = false;
            lastTime = time(nullptr);
        }
        bool operator < (const Port_t &rhs) const
        {
            return lastTime < rhs.lastTime;
        }

    } Port;
    typedef std::vector<Port> PortArr;

    typedef struct Quintet_t
    {
        Tins::IPv4Address srcIp, dstIP;
        uint16_t srcPort, dstPort;
        int protocol;

        Quintet_t()
        {}

        Quintet_t(const VpnServer::Quintet_t &quintet_)
        {
            *this = quintet_;
        }

        Quintet_t(Tins::IPv4Address srcIp_, uint16_t srcPort_, Tins::IPv4Address dstIp_, uint16_t dstPort_,
                  int protocol_)
        {
            srcIp = srcIp_, srcPort = srcPort_;
            dstIp_ = dstIp_, dstPort_ = dstPort_;
            protocol = protocol_;
        }
    } Quintet;

    // Объявление приватных переменных
    int tunFd_;
    int err_;
    std::string tunName_;
    int sockFd_;
    int epFd_;
    struct sockaddr_in vpnAddr_, clientAddr_;
    struct epoll_event evList_[EPOLL_SIZE];
    Tins::PacketSender sender_;
    Tins::IPv4Address localIp_, tunIp_;
    IpPortToSockaddr simpleSockaddr_;
    PortToAddr portToAddr_;
    AddrToPort addrToPort_;
    PortArr portArr_;
    std::pair<int, int> portRange_;

    // Объявление приватных методов
    void create_sock(const char *port);

    void create_tun();

    void create_epoll();

    void get_local_ip();

    void epoll_add(int fd, int status);

    void send_to_net(const Tins::IP &IpPack, const struct sockaddr_in &clientAddr);

    void send_to_client(const Tins::IP &IpPack);

    void write_tun(Tins::PDU &ip);

    void get_quintet(const Tins::IP &IpPack, Quintet &quintet);

    void print_quintet(const Quintet &quintet)
    {
        if (quintet.protocol == IPPROTO_TCP)
            std::cout << "TCP :";
        else if (quintet.protocol == IPPROTO_UDP)
            std::cout << "UDP :";
        else if(quintet.protocol == IPPROTO_ICMP)
            std::cout <<"ICMP :";
        std::cout << "src : " << quintet.srcIp.to_string() << ":" << quintet.srcPort << " -> "
                  << quintet.dstIP.to_string() << ":" << quintet.dstPort << std::endl;
    }

    bool get_ip_pack(const uint8_t *buf, int n, Tins::IP &pack);

    void mod_port(Tins::IP &pack, uint16_t value, int type);

public:
    VpnServer(const char *tunIp, const char *vpnPort, std::pair<int, int> portRange, const char *tunAddress, const char *ifName);

    void start();

    std::string get_tun_name() const
    {
        return tunName_;
    }

};
#endif //DBVPN_DBVPNCLIENT_H
