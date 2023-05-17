#include "dbvpnserver.h"
#include <iostream>
#include <sys/resource.h>
#include <vector>
#include <thread>
//
//void usage()
//{
//    std::cerr << "Usage:\n\t<tun_local_ip> (for example: 10.10.10.10)\n"
//              << "\t<udp_socket_port> (for example: 30000)\n"
//              << "\t<snat_port_range> (for example: 30000-34999)" << std::endl;
//    exit(0);
//}
//
//// increase_fd_limit увеличиванеи максимального лимита файлового дескриптора для процесса
//void increase_fd_limit() {
//    //структура rlimit для хранения текущих и максимальных лимитов дескриптора
//    struct rlimit rl;
//
//    // getrlimit() для получения текущих лимитов дескриптора и сохранения в 'rl'
//    // Если вызов функции успешен (возвращает 0)
//    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
//        // установка текущего лимита равным максимальному лимиту
//        rl.rlim_cur = rl.rlim_max;
//
//        // Вызов setrlimit() для обновления лимитов дескриптора с новыми значениями из 'rl'
//        // Если вызов функции неудачен (возвращает -1), вывод err
//        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
//            std::cerr << "Failed to get the file descriptor limit." << std::endl;
//        }
//    } else {
//        // Если вызов getrlimit() неудачен (возвращает -1), выводим сообщение об ошибке
//        std::cerr << "Failed to get the file descriptor limit." << std::endl;
//    }
//}
//
//
//int main(int argc, char **argv)
//{
//    if (argc != 6)
//    {
//        usage();
//    }
//
//    increase_fd_limit();
//// Создание экземпляра VPN-сервера используя tun_ip и порт
//    const char *tun_local_ip = argv[1];
//    const char *udp_socket_port = argv[2];
//    const char *snat_port_range = argv[3];
//    const char *tun_address = argv[4];
//    const char *if_name = argv[5];
//
//    std::pair<int, int> portRange;
//    sscanf(snat_port_range, "%d-%d", &portRange.first, &portRange.second);
//
//    VpnServer server(tun_local_ip, udp_socket_port, portRange, tun_address, if_name);
//    server.start();
//
//    return 0;
//}

void usage()
{
    std::cerr << "Usage:\n\t<tun_local_ip> (for example: 10.10.10.10)\n"
              << "\t<udp_socket_port> (for example: 30000)\n"
              << "\t<snat_port_range> (for example: 30000-34999)" << std::endl;
    exit(0);
}

void increase_fd_limit()
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
    {
        rl.rlim_cur = rl.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
        {
            std::cerr << "Failed to get the file descriptor limit." << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to get the file descriptor limit." << std::endl;
    }
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        usage();
    }
    increase_fd_limit();
    const char *tun_local_ip = argv[1];
    const char *udp_socket_port = argv[2];
    const char *snat_port_range = argv[3];
    const char *tun_address = argv[4];
    const char *if_name = argv[5];
    std::pair<int, int> portRange;
    sscanf(snat_port_range, "%d-%d", &portRange.first, &portRange.second);
    VpnServer server(tun_local_ip, udp_socket_port, portRange, tun_address, if_name);

    // Start the server in a separate thread for handling multiple connections
    std::thread serverThread([&]() {
        server.start();
    });

    serverThread.join();

    return 0;
}
