/*dbvpnserver.h и dbvpnserver.cpp : содержат реализацию VPN-сервера.
Здесь определен класс VpnServer.
Как и у  клиента, у него есть методы для создания устройства TUN,создания сокета и создания экземпляра epoll,
также есть методы для отправки пакетов клиенту (send_to_client) и в сеть (send_to_net).
Метод start - это то, где выполняется основной цикл сервера.
Он ожидает событий на устройстве TUN и сокете и пересылает пакеты между ними.
*/


// Реализация методов класса сервера VPN
#include "dbvpnserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <assert.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <iostream>
#include <tins/tins.h>

using namespace Tins;

VpnServer::VpnServer(const char *tunIp, const char *vpnPort, std::pair<int, int> portRange, const char *tunAddress, const char *ifName):
        sender_(ifName), tunIp_(tunIp), portRange_(portRange)
{
    get_local_ip(ifName);
    create_tun(tunAddress);
    create_sock(vpnPort);
    create_epoll();
}

void VpnServer::get_local_ip(const char *ifName)
{
    int inet_sock;
    struct ifreq ifr;
    char ip[32] = "";

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "wlp1s0");
    ioctl(inet_sock, SIOCGIFADDR, &ifr);
    strcpy(ip, inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
    localIp_ = IPv4Address(std::string(ip));
}

void VpnServer::create_tun(const char *tunAddress)
{
    struct ifreq ifr;

    err_ = 0;

    tunFd_ = open("/dev/net/tun", O_RDWR);
    assert(tunFd_ != -1);

    bzero(&ifr, sizeof(ifr));
    ifr.ifr_flags |= IFF_TUN | IFF_NO_PI;


    err_ = ioctl(tunFd_, TUNSETIFF, (void *)&ifr);
    assert(err_  != -1);

    tunName_ = std::string(ifr.ifr_name);

    std::string cmd = "";
    cmd += "ifconfig " + tunName_;
    cmd += " " + std::string(tunAddress) + " up";
    system(cmd.c_str());
}
void VpnServer::create_sock(const char *port)
{
    int sockfd = -1;
    struct sockaddr_in vpnAddr;

    bzero(&vpnAddr, sizeof(vpnAddr));
    vpnAddr.sin_family = AF_INET;
    vpnAddr.sin_port = htons(atoi(port));
    vpnAddr.sin_addr.s_addr = htons(INADDR_ANY);
    vpnAddr_ = vpnAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    assert(sockfd != -1);

    err_ = bind(sockfd, (struct sockaddr *)&vpnAddr, sizeof(vpnAddr));
    assert(err_ != -1);

    sockFd_ = sockfd;
}

void VpnServer::create_epoll()
{
    epFd_ = epoll_create1(0);

    assert(epFd_ != -1);

    epoll_add(sockFd_, EPOLLIN);
    epoll_add(tunFd_, EPOLLIN);

}

void VpnServer::epoll_add(int fd, int status)
{
    struct epoll_event ev;
    ev.events = status;
    ev.data.fd = fd;
    err_ = epoll_ctl(epFd_, EPOLL_CTL_ADD, fd, &ev);

    assert(err_ != -1);
}
/*
Функция get_ip_pack() пытается извлечь IP-пакет из предоставленного буфера и возвращает true в случае успеха.
*/
bool VpnServer::get_ip_pack(const uint8_t *buf, int n, IP &pack)
{
   try
   {
        pack = RawPDU(buf, n).to<IP>();
   }
   catch (...)
   {
        return false;
   }
   return true;
}
/*
Функция start() является основным циклом обработки событий VPN-сервера.
Он ожидает событий с помощью epoll_wait() и обрабатывает входящие и исходящие данные между устройством tun и сокетом.*/
void VpnServer::start()
{
    std::cout << "The " << get_tun_name() << " is running." << std::endl;
    std::cout << "Port range is " << portRange_.first << "-" << portRange_.second << std::endl;
    time_t nowTime = time(NULL);

    for(int i = portRange_.first+1; i<portRange_.second; i++)
        portArr_.push_back(Port(i));

    uint8_t buf[BUFF_SIZE];
    struct sockaddr_in tmpAddr;
    socklen_t sockLen = sizeof(struct sockaddr);
    long long cnt = 0;
    while(1)
    {
        int nfds = epoll_wait(epFd_, evList_, EPOLL_SIZE, -1);
        for(int i = 0; i<nfds; i++)
        {
            if(evList_[i].data.fd == tunFd_)
            {
                int nread = read(tunFd_, buf, sizeof(buf));

                IP oneIpPack;

                if(get_ip_pack(buf, nread, oneIpPack) == false)
					continue;
                send_to_client(oneIpPack);
            }
            else if(evList_[i].data.fd == sockFd_)
            {
                int nrecv =  recvfrom(sockFd_, buf, sizeof(buf), 0, (struct sockaddr *)&tmpAddr, &sockLen);

                IP oneIpPack;

				if(get_ip_pack(buf, nrecv, oneIpPack) == false)
					continue;

                send_to_net(oneIpPack, tmpAddr);
            }
        }
    }
}
/*
Функция mod_port() изменяет порт источника или назначения пакета TCP или UDP.
*/
void VpnServer::mod_port(Tins::IP &pack, uint16_t value, int type)
{
    if(pack.protocol() == IPPROTO_TCP && type == MOD_DST)
    {
        TCP *oneTcp = pack.find_pdu<TCP>();
        oneTcp->dport(value);
    }
    if(pack.protocol() == IPPROTO_UDP && type == MOD_DST)
    {
        UDP *oneUdp = pack.find_pdu<UDP>();
        oneUdp->dport(value);
    }
    if(pack.protocol() == IPPROTO_TCP && type == MOD_SRC)
    {
        TCP *oneTcp = pack.find_pdu<TCP>();
        oneTcp->sport(value);
    }
    if(pack.protocol() == IPPROTO_UDP && type == MOD_SRC)
    {
        UDP *oneUdp = pack.find_pdu<UDP>();
        oneUdp->sport(value);
    }

}
/*
Функция get_quintet() извлекает IP-адрес источника, порт источника, IP-адрес назначения, порт назначения и протокол из IP-пакета.
*/
void VpnServer::get_quintet(const Tins::IP &IpPack, Quintet &quintet)
{
    quintet.dstIP = IpPack.dst_addr(), quintet.srcIp = IpPack.src_addr();
    quintet.protocol = IpPack.protocol();
    if(IpPack.protocol() == IPPROTO_UDP)
    {
        const UDP *oneUdp = IpPack.find_pdu<UDP>();
        quintet.dstPort = oneUdp->dport();
        quintet.srcPort = oneUdp->sport();
    }
    else if(IpPack.protocol() == IPPROTO_TCP)
    {
        const TCP *oneTcp = IpPack.find_pdu<TCP>();
        quintet.dstPort = oneTcp->dport();
        quintet.srcPort = oneTcp->sport();
    }
    else if(IpPack.protocol() == IPPROTO_ICMP)
    {
        return ;
    }
}
/*
Функция write_tun() записывает сериализованный PDU на устройство tun.
*/
void VpnServer::write_tun(Tins::PDU &ip)
{
    char buf[BUFF_SIZE];
    std::vector<uint8_t> serV = ip.serialize();
    for(int i = 0; i<serV.size(); i++)
        buf[i] = serV[i];

    socklen_t sockLen = sizeof(struct sockaddr_in);

    write(tunFd_, buf, serV.size());
}
/*
Функция send_to_net() обрабатывает исходящие пакеты от устройства tun в сеть.
При необходимости он изменяет порт источника и отправляет пакет соответствующему получателю.
*/
void VpnServer::send_to_net(const Tins::IP &IpPack, const struct sockaddr_in &clientAddr)
{
    time_t nowTime = time(nullptr);
    IP oneIpPack = IpPack;
    Quintet quintet;
    get_quintet(IpPack, quintet);
    IpPort dstIpPort = std::make_pair(quintet.dstIP, quintet.dstPort);
    IpPort srcIpPort = std::make_pair(quintet.srcIp, quintet.srcPort);

    print_quintet(quintet);
    if(addrToPort_.count(srcIpPort) == 0)
    {
        for(auto &item : portArr_)
        {
            if(nowTime - item.lastTime >= 300 || item.used == false)
            {
                if(nowTime - item.lastTime >= 300)
                {
                    IpPort tmp = portToAddr_[item.port];
                    addrToPort_.erase(tmp);
                    portToAddr_.erase(item.port);
                    simpleSockaddr_.erase(tmp);
                }

                item.lastTime = nowTime;
                item.used = true;
                mod_port(oneIpPack, item.port, MOD_DST);
                addrToPort_[srcIpPort] = item.port;
                portToAddr_[item.port] = srcIpPort;
                simpleSockaddr_[srcIpPort] = clientAddr;
                srcIpPort.second = item.port;
                break;
            }
        }
    }
    else
    {
        uint16_t tmp = addrToPort_[srcIpPort];
        int seq = tmp - portRange_.first -1;
        portArr_[seq].lastTime = nowTime;
        mod_port(oneIpPack, tmp, 0);
    }

    oneIpPack.src_addr(tunIp_);
    write_tun(oneIpPack);
}
/*
Функция send_to_client() обрабатывает входящие пакеты из сети на устройство tun.
*/
void VpnServer::send_to_client(const Tins::IP &IpPack)
{
    char buf[BUFF_SIZE];
    IP oneIpPack = IpPack;
    Quintet quintet;
    get_quintet(IpPack, quintet);
    IpPort srcIpPort = std::make_pair(quintet.srcIp, quintet.srcPort);
    if(portToAddr_.count(quintet.dstPort) == 0)
    {
        return ;
    }
    IpPort dstIpPort = portToAddr_[quintet.dstPort];
    struct sockaddr_in tmpAddr = simpleSockaddr_[dstIpPort];

    oneIpPack.dst_addr(dstIpPort.first);

    mod_port(oneIpPack, dstIpPort.second, MOD_SRC);

    std::vector<uint8_t> serV = oneIpPack.serialize();

    for (int i = 0; i < serV.size(); i++)
        buf[i] = serV[i];
    get_quintet(oneIpPack, quintet);
    socklen_t sockLen = sizeof(struct sockaddr_in);
    sendto(sockFd_, buf, serV.size(), 0,(struct sockaddr *)&tmpAddr, sockLen);
}
