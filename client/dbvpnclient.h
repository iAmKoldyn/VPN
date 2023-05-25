/*
dbvpnclient.h и dbvpnclient.cpp : содержат реализацию VPN-клиента.
Класс TunDev определен здесь. В нем есть методы для создания устройства TUN (create_tun),
создания сокета (create_sock) и создания экземпляра epoll (create_epoll).
Метод start - это то, где выполняется основной цикл клиента.
Он ожидает событий на устройстве TUN и сокете и пересылает пакеты между ними.
*/

#ifndef DBVPN_DBVPNCLIENT_H
#define DBVPN_DBVPNCLIENT_H
#define IP_SIZE 128
#define IF_NAME_SIZE 16
#define BUFF_SIZE 2048
#define EPOLL_SIZE 16
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>

class TunDev
{
private:
    int tunFd_;
    int err_;
    std::string tunName_;
    int sockFd_;
    int vpnPort_;
    int epFd_;
    std::string vpnIp_;
    struct sockaddr_in vpnAddr_;

    struct epoll_event evList_[EPOLL_SIZE];

    void create_sock(const char *port);

    void create_tun();

    void create_epoll();

    void epoll_add(int fd, int status);

public:

    TunDev(const char *vpnIp, const char *vpnPort);

    void start();

    std::string getTunNanme() const
    {
        return tunName_;
    }

};

#endif //DBVPN_DBVPNCLIENT_H

