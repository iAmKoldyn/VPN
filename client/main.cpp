/*
main.cpp (client and server): точка входа.
Для клиента он проверяет, указаны ли IP и порт в качестве аргументов командной строки.
Если это так, он создает объект TunDev и запускает его.
Для сервера он увеличивает ограничение на файловый дескриптор,
анализирует аргументы командной строки и запускает VPN-сервер.
*/

#include "dbvpnclient.h"

#include <iostream>

using namespace std;

int main(int argv, char *argc[])
{

    if (argv > 2)
    {
        TunDev tun(argc[1], argc[2]);

        cout << "The " << tun.getTunNanme() << " is running." << endl;

        tun.start();

    }
    else
    {
        printf("Input IP Port!\n");
    }
    return 0;
}
