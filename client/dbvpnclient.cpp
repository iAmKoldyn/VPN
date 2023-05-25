/*
dbvpnclient.h и dbvpnclient.cpp : содержат реализацию VPN-клиента.
Класс TunDev определен здесь. В нем есть методы для создания устройства TUN (create_tun),
создания сокета (create_sock) и создания экземпляра epoll (create_epoll).
Метод start - это то, где выполняется основной цикл клиента.
Он ожидает событий на устройстве TUN и сокете и пересылает пакеты между ними.
*/

#include "dbvpnclient.h"
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

/*
Конструктор TunDev(const char *vpnIp, const char *vpnPort) инициализирует объект TunDev.
Он принимает IP-адрес VPN и порт в качестве параметров.
*/
TunDev::TunDev(const char *vpnIp, const char *vpnPort):
    vpnIp_(vpnIp)
{
    create_tun();
    create_sock(vpnPort);
    create_epoll();

    bzero(&vpnAddr_, sizeof(vpnAddr_));
    vpnAddr_.sin_family = AF_INET;
    vpnAddr_.sin_port = htons(atoi(vpnPort));
    inet_pton(AF_INET, vpnIp, &vpnAddr_.sin_addr);

}

/*
Функция create_tun() создает виртуальное сетевое устройство, используя интерфейс /dev/net/tun.
Он открывает файл устройства, устанавливает флаги интерфейса и настраивает интерфейс с помощью ifconfig.
*/
void TunDev::create_tun()
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
    cmd += "ifconfig " + tunName_ + " mtu 1300" + " up";
    system(cmd.c_str());
    system("route add default dev tun0");
 }

/*
Функция create_sock(const char *port) создает сокет для обмена данными.
Он настраивает UDP-сокет и привязывает его к указанному VPN-адресу и порту.
*/
void TunDev::create_sock(const char *port)
{
    int sockfd = -1;

    struct sockaddr_in vpnAddr;

    bzero(&vpnAddr, sizeof(vpnAddr));
    vpnAddr.sin_family = AF_INET;
    vpnAddr.sin_port = htons(atoi(port));
    vpnAddr.sin_addr.s_addr = htons(INADDR_ANY);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    assert(sockfd != -1);

    err_ = bind(sockfd, (struct sockaddr *)&vpnAddr, sizeof(vpnAddr));

    if (err_ == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    sockFd_ = sockfd;
}

/*
Функция create_epoll() создает экземпляр epoll и добавляет к нему дескриптор сокета и файла tun.
*/
void TunDev::create_epoll()
{
    epFd_ = epoll_create1(0);

    assert(epFd_ != -1);

    epoll_add(sockFd_, EPOLLIN);
    epoll_add(tunFd_, EPOLLIN);

}

/*
Функция epoll_add(int fd, int status) добавляет файловый дескриптор к экземпляру epoll с указанным статусом.
*/
void TunDev::epoll_add(int fd, int status)
{
    struct epoll_event ev;
    ev.events = status;
    ev.data.fd = fd;
    err_ = epoll_ctl(epFd_, EPOLL_CTL_ADD, fd, &ev);

    assert(err_ != -1);
}

/*
Функция start() является основным циклом обработки событий VPN-клиента. 
Он ожидает событий с помощью epoll_wait() и обрабатывает входящие и исходящие данные между устройством tun и сокетом.
*/
void TunDev::start()
{
    uint8_t buf[BUFF_SIZE];
    struct sockaddr tmpAddr;
    socklen_t sockLen = sizeof(struct sockaddr);
    while(1)
    {
        int nfds = epoll_wait(epFd_, evList_, EPOLL_SIZE, -1);

        for(int i = 0; i<nfds; i++)
        {
            if(evList_[i].data.fd == tunFd_)
            {
                int nread =  read(tunFd_, buf, sizeof(buf));
                const void *peek = buf;
                const struct iphdr *iphdr = static_cast<const struct iphdr *>(peek);
                if(iphdr->version == 4)
                {
                    sendto(sockFd_, buf, nread, 0, (struct sockaddr *)&vpnAddr_, sockLen);
                }
            }
            else if(evList_[i].data.fd == sockFd_)
            {
                int nrecv =  recvfrom(sockFd_, buf, sizeof(buf), 0, &tmpAddr, &sockLen);
                write(tunFd_, buf, nrecv);
            }
        }
    }
}
