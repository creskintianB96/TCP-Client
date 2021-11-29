// TCP_Client_console.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

#ifdef _WIN64
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <cstdio>
#include <windows.h>

// Need to link with Ws2_32.lib
// (#pragma indica che al linker che � necessaria la lib specificata dopo)
#pragma comment(lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#define SERVER_TCP_PORT 54000
#define SERVER_IP_ADDRESS "192.168.60.1"
// #define SERVER_IP_ADDRESS   "192.168.195.133"		//10.4.1.35
// #define SERVER_IP_ADDRESS   "127.0.0.1"

// #define VEC_NUM_ELEMENT     1680

#ifdef _WIN64
#define SOCKET_ERROR_ SOCKET_ERROR
#else
#define SOCKET_ERROR_ -1
#endif

int main()
{
    std::string ipAddress = SERVER_IP_ADDRESS; // IP Address of the server
    int port = SERVER_TCP_PORT;                // Listening port # on the server

#ifdef _WIN64
    // Initialize WinSock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        std::cerr << "> Can't start Winsock, Err #" << wsResult << std::endl;
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::err << "> Can't create socket, Err #" << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        std::cerr << "> Can't create socket, Error!" << std::endl;
        return 1;
    }
#endif

    // Fill in a hint structure
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
    //hint.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

    // Connect to server
    int connectRes = connect(sock, (sockaddr *)&hint, sizeof(hint));
    if (connectRes == SOCKET_ERROR_)
    {
#ifdef _WIN64
        std::cerr << "> Can't connect to server, Err #" << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
#else
        std::cerr << "> Can't connect to server, Error!" << std::endl;
        close(sock);
#endif

        return 1;
    }

    else if (connectRes == 0)
    {
        std::cout << "> Connesso al Server con esito " << connectRes << std::endl;
    }
    

    char buf[4096];

    std::cout << "> Open trajectory file" << std::endl;

#ifdef _WIN64
    std::ifstream infile("desired_trajectory.txt", std::ios::in);
#else
    std::ifstream infile("/home/cristian/cristian_ws/src/robish/fanuc_planning/debug/desired_trajectory.txt", std::ios::in);
#endif

    std::vector<float> joints_buffer;

    //check to see that the file was opened correctly:
    if (!infile.is_open())
    {
        std::cerr << "> There was a problem opening the input file!\n";
        exit(1); //exit or do additional error checking
    }

    float num = 0.0;

    std::cout << "> Acquire all joint values from trajectory file" << std::endl;
    //keep storing values from the text file so long as data exists:
    while (infile >> num)
    {
        joints_buffer.push_back(num);
    }

    int dimension = joints_buffer.size();
    std::cout << "> Acquired " << dimension << " values" << std::endl;
    //verify that the scores were stored correctly:
    // for (int i = 0; i < joints_buffer.size(); ++i) {
    //     std::cout << joints_buffer[i] << std::endl;
    // }

    bool inviare = true;

    while(inviare)
    {
        std::cout << "> Invio i dati al Robot" << std::endl;

        int sendResult = send(sock, (char *)joints_buffer.data(), dimension * sizeof(float), 0);
        //int sendResult = send(sock, (char*)prova_traiettoria.data(), VEC_NUM_ELEMENT * sizeof(float), 0);
        //int sendResult = send(sock, (char*)prova_traiettoria_rad.data(), VEC_NUM_ELEMENT * sizeof(float), 0);

        if (sendResult == SOCKET_ERROR_)
        {
            std::cout << "> Could not send to server! Whoops!\r" << std::endl;
            continue;
        }

        // Wait for response
        memset(buf, 0, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);
        if (bytesReceived == SOCKET_ERROR_)
        {
            std::cout << "> There was an error getting response from server\r" << std::endl;
        }
        else
        {
            //Display response to console
            // std::cout << "SERVER> " << std::string(buf, bytesReceived) << std::endl;
            std::string recv_string = std::string(buf, bytesReceived);
            double recv_double = atoi(recv_string.c_str());
            std::cout << "SERVER> " << recv_double << std::endl; 

            if (recv_double == 0)
                break;
        }

    }

    // Close the socket
#ifdef _WIN64
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 0;
}
