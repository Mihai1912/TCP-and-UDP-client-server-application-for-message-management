#include <iostream>
#include "helpers.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <unordered_map>
#include <sstream>
#include <math.h>


#define MAX_NO_CLIENTS 10

using namespace std;

struct client{
    char id[10];
    unordered_map<string , int> topic_sf;
};

struct udp_msg {
    char topic[50];
    char type;
    char content[1500];
};


void send_to_client( client subs[10] , int subs_no , char *msg , unordered_map<string , bool> online , unordered_map<string , int> all_time_users) {
    for (int i = 0; i < subs_no; i++)
    {
        if (online.find(subs[i].id)->second) {
            int sock = all_time_users.find(subs[i].id)->second;
            int send_ret = send(sock, msg, 1600, 0);
            DIE(send_ret < 0, "exit");
            // cout << "trimis" << endl;
        }
    }
}


int main(int argc , char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT_DORIT>\n", argv[0]);
        exit(0);
    }

    int port_number = atoi(argv[1]);

    int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    DIE(TCP_socket < 0, "socket TCP");
    int UDP_socket = socket(PF_INET, SOCK_DGRAM, 0);  //  UDP
    DIE(UDP_socket < 0, "socket UDP");

    int enable = 1;
    if (setsockopt(TCP_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt(UDP_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in *adr_sv = (struct sockaddr_in*)calloc(1 , sizeof(struct sockaddr_in));
    adr_sv->sin_addr.s_addr = INADDR_ANY;
    adr_sv->sin_family = AF_INET;
    adr_sv->sin_port = htons(port_number);

    int TCP_s_ret = bind(TCP_socket, (struct sockaddr *)adr_sv, sizeof(struct sockaddr));
    DIE(TCP_s_ret < 0, "bind TCP");

    int UDP_s_ret = bind(UDP_socket, (struct sockaddr *)adr_sv, sizeof(struct sockaddr));
    DIE(UDP_s_ret < 0, "bind UDP");

    struct pollfd fds[32];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = TCP_socket;
    fds[1].events = POLLIN;
    fds[2].fd = UDP_socket;
    fds[2].events = POLLIN;
    int sock_no = 3;

    int listen_ret = listen(TCP_socket , MAX_NO_CLIENTS);
    DIE(listen_ret < 0 , "listen tcp");

    int rc;
    socklen_t clen;
    struct sockaddr_in c_addr , adr_cli;
    int new_sock_tcp;
    unordered_map<string , int> all_time_users;
    unordered_map<string , int> active_users_id_socket;
    unordered_map<int , string> active_users_socket_id;
    unordered_map<string , bool> online_users;
    client subs_vect[10];
    int no_of_subs = 0;

    while (1)
    {
        char* buff = (char*)calloc(1600 , sizeof(char));

        rc = poll(fds, sock_no, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < sock_no; i++) {

            if (fds[i].revents & POLLIN) {

                if (fds[i].fd == STDIN_FILENO) {

                    fgets(buff, 255, stdin);
                    if (!strncmp(buff , "exit\n" , 5)) {

                        close(TCP_socket);
                        close(UDP_socket);

                        for (int i = 0; i < sock_no; i++)
                        {
                            close(fds[i].fd);
                        }
                        
                    return 0;
                    } else {
                        printf("Serverul va accepta, de la tastatura, doar comanda exit\n");
                    }

                } else if (fds[i].fd == TCP_socket) {

                    socklen_t clen = sizeof(struct sockaddr_in); 
                    new_sock_tcp = accept(TCP_socket, (struct sockaddr *)&c_addr, &clen);
                    DIE(new_sock_tcp < 0, "accept");

                    fds[sock_no].fd = new_sock_tcp;
                    fds[sock_no].events = POLLIN;
                    sock_no++;

                    memset(buff, 0, 256);
                    int n = recv(new_sock_tcp, buff, 256, 0);
                    DIE(n < 0, "recv");

                    if (all_time_users.count(buff) == 0) {
                        all_time_users.insert(make_pair(buff , new_sock_tcp));
                        active_users_id_socket.insert(make_pair(buff , new_sock_tcp));
                        active_users_socket_id.insert(make_pair(new_sock_tcp , buff));
                        online_users.insert(make_pair(buff , true));
                        strcpy(subs_vect[no_of_subs].id , buff);
                        no_of_subs++;
                        
                        cout << "New client " << buff << " connected from " << inet_ntoa(c_addr.sin_addr) << ":" <<  htons(c_addr.sin_port) << endl;
                    } else {
                        if (online_users[buff] == true) {
                            cout << "Client " << buff << " already connected." << endl;
                            close(new_sock_tcp);
                        } else {
                            all_time_users[buff] = new_sock_tcp;
                            active_users_socket_id[all_time_users[buff]] = buff;
                            active_users_id_socket[buff] = new_sock_tcp;
                            online_users[buff] = true;
                            cout << "New client " << buff << " connected from " << inet_ntoa(c_addr.sin_addr) << ":" <<  htons(c_addr.sin_port) << endl;
                        }
                    }
                    continue;
                } else if (fds[i].fd == UDP_socket) {
                    
                    memset(buff, 0, 1600);

                
                    socklen_t socklen = sizeof(struct sockaddr);
                    int n = recvfrom(UDP_socket , buff , 1600 , 0 , (struct sockaddr *)&adr_cli, &socklen);
                    DIE(n < 0, "recv");

                    // udp_msg* msg = (udp_msg*)calloc(1,sizeof(udp_msg));
                    
                    // memcpy(msg->topic , buff , 50);
                    // memcpy(&msg->type , buff+50 , 1);
                    // memcpy(msg->content , buff+51 , 1500);

                    send_to_client(subs_vect , no_of_subs , buff , online_users , all_time_users);

                    // switch ((int)msg->type) {
                    // case 0:

                    //     if ((*(uint8_t *)msg->content) == 0) {
                    //         string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //         + msg->topic + string(" - ") + string("INT - ") + to_string(ntohl(*(uint32_t*)(msg->content+1)));
                    //         send_to_client(subs_vect , no_of_subs , buff , online_users , all_time_users);
                    //     } else {
                    //         string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //         + msg->topic + string(" - ") + string("INT - -") + to_string(ntohl(*(uint32_t*)(msg->content+1)));
                    //     }
                    //     break;
                    // case 1:
                    //     if (to_string(ntohs(*(uint16_t*)(msg->content)) % 100).length() == 1) {
                    //         string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //         + msg->topic + string(" - ") + string("SHORT_REAL - ") + to_string(ntohs(*(uint16_t*)(msg->content))/100)
                    //         + string(".0") + to_string(ntohs(*(uint16_t*)(msg->content)) % 100);
                    //     } else {
                    //         string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //         + msg->topic + string(" - ") + string("SHORT_REAL - ") + to_string(ntohs(*(uint16_t*)(msg->content))/100)
                    //         + string(".") + to_string(ntohs(*(uint16_t*)(msg->content)) % 100);
                    //     }
                    //     break;
                    // case 2:
                    
                    //     if ((*(uint8_t *)msg->content) == 0) {
                    //         uint8_t power = (*(uint8_t *)(msg->content + sizeof(uint8_t) + sizeof(uint32_t)));
                    //         uint32_t no = ntohl(*(uint32_t *)(msg->content + sizeof(uint8_t)));
                    //         int aux = pow(10 , power);
                    //         if (to_string(no % aux).length() < power) {
                    //             string decimal = to_string(no % aux);
                    //             for (int i = to_string(no % aux).length(); i < power; i++)
                    //             {
                    //                 decimal = "0" + decimal;
                    //             }
                    //             string snd =  string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //             + msg->topic + string(" - ") + string("FLOAT - ") + to_string(no / aux)
                    //             + string(".") + decimal;
                    //         } else {
                    //             string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //             + msg->topic + string(" - ") + string("FLOAT - ") + to_string(no / aux)
                    //             + string(".") + to_string(no % aux);
                    //         }

                    //     } else {
                    //         uint8_t power = (*(uint8_t *)(msg->content + sizeof(uint8_t) + sizeof(uint32_t)));
                    //         uint32_t no = ntohl(*(uint32_t *)(msg->content + sizeof(uint8_t)));
                    //         int aux = pow(10 , power);
                    //         if (to_string(no % aux).length() < power) {
                    //             string decimal = to_string(no % aux);
                    //             for (int i = to_string(no % aux).length(); i < power; i++)
                    //             {
                    //                 decimal = "0" + decimal;
                    //             }
                    //             string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //             + msg->topic + string(" - ") + string("FLOAT - -") + to_string(no / aux)
                    //             + string(".") + decimal;
                    //         } else {
                    //             string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //             + msg->topic + string(" - ") + string("FLOAT - -") + to_string(no / aux)
                    //             + string(".") + to_string(no % aux);
                    //         }
                    //     }
                    //     break;

                    // default:
                    //     string snd = string(inet_ntoa(adr_cli.sin_addr)) + string(":") + to_string(htons(adr_cli.sin_port)) + string(" - ") 
                    //     + msg->topic + string(" - ") + string("STRING - ") + msg->content;
                    //     break;
                    // }
                } else {
                    memset(buff, 0, 256);
                    int ret = recv(fds[i].fd, buff, 256, 0);

                    if (!strncmp(buff , "exit" , 4)) {
                        cout << "Client " << string(active_users_socket_id.find(fds[i].fd)->second) << " disconnected." << endl;
                        active_users_id_socket.erase(active_users_socket_id[fds[i].fd]);
                        online_users[active_users_socket_id[fds[i].fd]] = false;
                        active_users_socket_id.erase(fds[i].fd);
                        close(fds[i].fd);
                    } else if (!strncmp(buff , "subscribe" , 9)) {
                        string aux(buff);
                        stringstream a(aux);
                        string cmd , topic;
                        int sf;
                        a >> cmd >> topic >> sf;
                        
                        string id = active_users_socket_id[fds[i].fd];
                        for (int i = 0; i < no_of_subs; i++)
                        {
                            if (!strcmp(subs_vect[i].id , id.c_str())) {
                                subs_vect[i].topic_sf.insert(make_pair(topic , sf));
                            }
                        }
                    } else if (!strncmp(buff , "unsubscribe" , 11)) {
                        string aux(buff);
                        stringstream a(aux);
                        string cmd , topic;
                        a >> cmd >> topic;

                        string id = active_users_socket_id[fds[i].fd];
                        for (int i = 0; i < no_of_subs; i++)
                        {
                            if (!strcmp(subs_vect[i].id , id.c_str())) {
                                subs_vect[i].topic_sf.erase(topic);
                            }
                        }
                        
                        
                    }
                }
            }
        }
    }
    return 0;
}