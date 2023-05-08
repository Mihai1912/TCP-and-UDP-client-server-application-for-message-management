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
#include <vector>
#include <netinet/tcp.h>



#define MAX_NO_CLIENTS 10

using namespace std;

struct client{
    char id[10];
    unordered_map<string , int> topic_sf;
    vector<char*> msg_to_send;
    int no_of_unsent_msg;
};

struct udp_msg {
    char topic[50];
    char type;
    char content[1500];
};

int init_socket_TCP (int port_number) {
    int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    DIE(TCP_socket < 0, "socket TCP");

    int enable = 1;
    if (setsockopt(TCP_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in *adr_sv = (struct sockaddr_in*)calloc(1 , sizeof(struct sockaddr_in));
    adr_sv->sin_addr.s_addr = INADDR_ANY;
    adr_sv->sin_family = AF_INET;
    adr_sv->sin_port = htons(port_number);

    int TCP_s_ret = bind(TCP_socket, (struct sockaddr *)adr_sv, sizeof(struct sockaddr));
    DIE(TCP_s_ret < 0, "bind TCP");

    int listen_ret = listen(TCP_socket , MAX_NO_CLIENTS);
    DIE(listen_ret < 0 , "listen tcp");

    return TCP_socket;
}

int init_socket_UDP (int port_number) {
    int UDP_socket = socket(PF_INET, SOCK_DGRAM, 0);  //  UDP
    DIE(UDP_socket < 0, "socket UDP");

    int enable = 1;
    if (setsockopt(UDP_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in *adr_sv = (struct sockaddr_in*)calloc(1 , sizeof(struct sockaddr_in));
    adr_sv->sin_addr.s_addr = INADDR_ANY;
    adr_sv->sin_family = AF_INET;
    adr_sv->sin_port = htons(port_number);

    int UDP_s_ret = bind(UDP_socket, (struct sockaddr *)adr_sv, sizeof(struct sockaddr));
    DIE(UDP_s_ret < 0, "bind UDP");
    
    return UDP_socket;
}

void send_to_client( client subs[10] , int subs_no , char *msg , unordered_map<string , bool> online , unordered_map<string , int> all_time_users) {
    
    udp_msg* m = (udp_msg*)calloc(1,sizeof(udp_msg));
                    
    memcpy(m->topic , msg , 50);
    memcpy(&m->type , msg+50 , 1);
    memcpy(m->content , msg+51 , 1500);

    // Se parcurge toata lista de clienti
    for (int i = 0; i < subs_no; i++)
    {
        // client online
        if (online.find(subs[i].id)->second) {
            // se verifica daca acesta este abonat la topicul curent
            if (subs[i].topic_sf.count(m->topic) >= 1) {
                int sock = all_time_users.find(subs[i].id)->second;
                int send_ret = send(sock, msg, 1600, 0);
                DIE(send_ret < 0, "exit");
            }
        } 
        // client offline
        else {
            // se verifica daca este a bonat si doreste sa primeasca toate mesajele de la topicul curent
            if (subs[i].topic_sf.count(m->topic) >= 1 && subs[i].topic_sf.find(m->topic)->second) {
                subs[i].msg_to_send.push_back(msg);
                subs[i].no_of_unsent_msg++;
            }
        }
    }
}

void sending_lost_msg(int sock , int no_of_subs , client *subs_vect , char *buff) {
    for (int i = 0; i < no_of_subs; i++) {
        if (!strcmp(subs_vect[i].id , buff)) {
            if (subs_vect[i].no_of_unsent_msg != 0) {
                for (int j = 0; j < subs_vect[i].no_of_unsent_msg; j++)
                {
                    char *msg = (char*)calloc(1600 , sizeof(char));
                    memcpy(msg , subs_vect[i].msg_to_send.front() , 1600);
                    subs_vect[i].msg_to_send.erase(subs_vect[i].msg_to_send.begin());
                    send(sock, msg, 1600, 0);
                }
            }
        }
    }
}

void run_sv(int TCP_socket , int UDP_socket){

    int new_sock_tcp;
    int no_of_subs = 0;
    int sock_no = 3;
    struct sockaddr_in c_addr , adr_cli;
    unordered_map<string , int> all_time_users;
    unordered_map<string , int> active_users_id_socket;
    unordered_map<int , string> active_users_socket_id;
    unordered_map<string , bool> online_users;
    client subs_vect[10];

    struct pollfd fds[32];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = TCP_socket;
    fds[1].events = POLLIN;
    fds[2].fd = UDP_socket;
    fds[2].events = POLLIN;

    while (1)
    {
        char* buff = (char*)calloc(1600 , sizeof(char));

        int rc = poll(fds, sock_no, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < sock_no; i++) {

            if (fds[i].revents & POLLIN) {

                // STDIN
                if (fds[i].fd == STDIN_FILENO) {

                    fgets(buff, 255, stdin);
                    if (!strncmp(buff , "exit\n" , 5)) {

                        close(TCP_socket);
                        close(UDP_socket);

                        for (int i = 0; i < sock_no; i++) {
                            close(fds[i].fd);
                        }
                    return;
                    }

                } 
                // TCP
                else if (fds[i].fd == TCP_socket) {

                    socklen_t clen = sizeof(struct sockaddr_in); 
                    new_sock_tcp = accept(TCP_socket, (struct sockaddr *)&c_addr, &clen);
                    DIE(new_sock_tcp < 0, "accept");

                    int enable = 1;
                    setsockopt(new_sock_tcp, SOL_SOCKET, TCP_NODELAY, &enable, sizeof(int)) ;

                    fds[sock_no].fd = new_sock_tcp;
                    fds[sock_no].events = POLLIN;
                    sock_no++;

                    memset(buff, 0, 256);
                    int n = recv(new_sock_tcp, buff, 256, 0);
                    DIE(n < 0, "recv");

                    // Prima conectare a clientului
                    if (all_time_users.count(buff) == 0) {

                        all_time_users.insert(make_pair(buff , new_sock_tcp));
                        active_users_id_socket.insert(make_pair(buff , new_sock_tcp));
                        active_users_socket_id.insert(make_pair(new_sock_tcp , buff));
                        online_users.insert(make_pair(buff , true));
                        strcpy(subs_vect[no_of_subs].id , buff);
                        subs_vect[no_of_subs].no_of_unsent_msg = 0;
                        no_of_subs++;
                        
                        cout << "New client " << buff << " connected from " << inet_ntoa(c_addr.sin_addr) << ":" <<  htons(c_addr.sin_port) << endl;
                    } 
                    // Reconectare client
                    else {
                        // Un alt client incearca sa se conecteze cu un id care este deja conectat
                        if (online_users[buff] == true) {
                            cout << "Client " << buff << " already connected." << endl;
                            close(new_sock_tcp);
                        } 
                        // Reconectare client
                        else {
                            all_time_users[buff] = new_sock_tcp;
                            active_users_socket_id[all_time_users[buff]] = buff;
                            active_users_id_socket[buff] = new_sock_tcp;
                            online_users[buff] = true;

                            sending_lost_msg(new_sock_tcp , no_of_subs , subs_vect , buff);
                            
                            cout << "New client " << buff << " connected from " << inet_ntoa(c_addr.sin_addr) << ":" <<  htons(c_addr.sin_port) << endl;
                        }
                    }
                } 
                // UDP
                else if (fds[i].fd == UDP_socket) {
                    
                    memset(buff, 0, 1600);

                
                    socklen_t socklen = sizeof(struct sockaddr);
                    int n = recvfrom(UDP_socket , buff , 1600 , 0 , (struct sockaddr *)&adr_cli, &socklen);
                    DIE(n < 0, "recv");
                    send_to_client(subs_vect , no_of_subs , buff , online_users , all_time_users);

                } 
                // Alte comenzi ale abonatului
                else {
                    memset(buff, 0, 256);
                    recv(fds[i].fd, buff, 256, 0);
                    // exit
                    if (!strncmp(buff , "exit" , 4)) {
                        cout << "Client " << string(active_users_socket_id.find(fds[i].fd)->second) << " disconnected." << endl;
                        active_users_id_socket.erase(active_users_socket_id[fds[i].fd]);
                        online_users[active_users_socket_id[fds[i].fd]] = false;
                        active_users_socket_id.erase(fds[i].fd);
                        close(fds[i].fd);
                    } 
                    // subscribe
                    else if (!strncmp(buff , "subscribe" , 9)) {
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
                    } 
                    // unsubscribe
                    else if (!strncmp(buff , "unsubscribe" , 11)) {
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
}


int main(int argc , char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT_DORIT>\n", argv[0]);
        exit(0);
    }

    int port_number = atoi(argv[1]);

    int TCP_socket = init_socket_TCP(port_number);
    int UDP_socket = init_socket_UDP(port_number);

    run_sv(TCP_socket , UDP_socket);
    
    return 0;
}