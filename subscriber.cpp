#include <iostream>
#include "helpers.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sstream>
#include <poll.h>
#include <math.h>

using namespace std;

struct udp_msg {
    char topic[50];
    char type;
    char content[1500];
};

int main(int argc , char *argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_Server> <Port_Server>\n",argv[0]);
        return 0;
    }

    if (strlen(argv[1]) > 10) {
        fprintf(stderr, "The ID must be a maximum of 10 characters \n");
        return 0;
    }

    int port_number = atoi(argv[3]);

    struct sockaddr_in *adr_sv = (struct sockaddr_in*)calloc(1 , sizeof(struct sockaddr_in));
    adr_sv->sin_family = AF_INET;
    adr_sv->sin_port = htons(port_number);
    inet_pton(AF_INET , argv[2] , &adr_sv->sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock < 0, "socket");

    int connect_ret = connect(sock, (struct sockaddr *)adr_sv, sizeof(struct sockaddr_in));
    DIE(connect_ret < 0, "connect");

    int send_ret = send(sock, argv[1], strlen(argv[1]), 0);
    DIE(send_ret < 0, "send");

    struct pollfd fds[2];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    char* msg_recv = (char*)calloc(1600 , sizeof(char));
    while (1)
    {
        
        int ready = poll(fds, 2, -1);
        DIE(ready < 0, "poll");


        if (fds[0].revents & POLLIN) {

            memset(msg_recv , 0 , 1600);

            int rc = recv(sock, msg_recv, 1600 , 0);
            if (rc <= 0) {
                return 0;
            } else if (rc >= 1600) {
                udp_msg* msg = (udp_msg*)calloc(1,sizeof(udp_msg));
                    
                memcpy(msg->topic , msg_recv , 50);
                memcpy(&msg->type , msg_recv + 50 , 1);
                memcpy(msg->content , msg_recv + 51 , 1500);

                switch ((int)msg->type) {
                    case 0:
                        if ((*(uint8_t *)msg->content) == 0) {
                            string snd = msg->topic + string(" - ") + string("INT - ") + to_string(ntohl(*(uint32_t*)(msg->content+1)));
                            cout << snd << endl;
                        } else {
                            string snd = msg->topic + string(" - ") + string("INT - -") + to_string(ntohl(*(uint32_t*)(msg->content+1)));
                            cout << snd << endl;
                        }
                        break;
                    case 1:
                        if (to_string(ntohs(*(uint16_t*)(msg->content)) % 100).length() == 1) {
                            string snd = msg->topic + string(" - ") + string("SHORT_REAL - ") + to_string(ntohs(*(uint16_t*)(msg->content))/100)
                            + string(".0") + to_string(ntohs(*(uint16_t*)(msg->content)) % 100);
                            cout << snd << endl;
                        } else {
                            string snd =  msg->topic + string(" - ") + string("SHORT_REAL - ") + to_string(ntohs(*(uint16_t*)(msg->content))/100)
                            + string(".") + to_string(ntohs(*(uint16_t*)(msg->content)) % 100);
                            cout << snd << endl;
                        }
                        break;
                    case 2:
                    
                        if ((*(uint8_t *)msg->content) == 0) {
                            uint8_t power = (*(uint8_t *)(msg->content + sizeof(uint8_t) + sizeof(uint32_t)));
                            uint32_t no = ntohl(*(uint32_t *)(msg->content + sizeof(uint8_t)));
                            int aux = pow(10 , power);
                            if (to_string(no % aux).length() < power) {
                                string decimal = to_string(no % aux);
                                for (int i = to_string(no % aux).length(); i < power; i++)
                                {
                                    decimal = "0" + decimal;
                                }
                                string snd =   msg->topic + string(" - ") + string("FLOAT - ") + to_string(no / aux)
                                + string(".") + decimal;
                                cout << snd << endl;
                            } else {
                                string snd = msg->topic + string(" - ") + string("FLOAT - ") + to_string(no / aux)
                                + string(".") + to_string(no % aux);
                                cout << snd << endl;
                            }

                        } else {
                            uint8_t power = (*(uint8_t *)(msg->content + sizeof(uint8_t) + sizeof(uint32_t)));
                            uint32_t no = ntohl(*(uint32_t *)(msg->content + sizeof(uint8_t)));
                            int aux = pow(10 , power);
                            if (to_string(no % aux).length() < power) {
                                string decimal = to_string(no % aux);
                                for (int i = to_string(no % aux).length(); i < power; i++)
                                {
                                    decimal = "0" + decimal;
                                }
                                string snd =  msg->topic + string(" - ") + string("FLOAT - -") + to_string(no / aux)
                                + string(".") + decimal;
                                cout << snd << endl;
                            } else {
                                string snd =  msg->topic + string(" - ") + string("FLOAT - -") + to_string(no / aux)
                                + string(".") + to_string(no % aux);
                                cout << snd << endl;
                            }
                        }
                        break;

                    default:
                        string snd =  msg->topic + string(" - ") + string("STRING - ") + msg->content;
                        cout << snd << endl;
                        break;
                    }
            }

            

        } else if (fds[1].revents & POLLIN) {

            char* buff = (char*)calloc(256 , sizeof(char));
            fgets(buff, 255, stdin);

            if (!strncmp(buff , "exit" , 4)) {
                int send_ret = send(sock, buff, 256, 0);
                DIE(send_ret < 0, "exit");
                close(sock);
                return 0;
            } 
            else if (!strncmp(buff , "subscribe" , 9)) {

                string aux(buff);
                stringstream a(aux);
                string cmd , topic;
                int sf;
                a >> cmd >> topic >> sf;

                if (topic.length() > 51 || (sf != 0 && sf != 1)) {
                    printf("Subscribe usage: subscribe topic SF\n");
                } else {
                    string s_to_send =cmd + " " + topic + " " + (char)(sf+48);
                    int len = cmd.length() + topic.length() + 3;
                    send(sock, s_to_send.c_str() , len, 0);
                    cout << "Subscribed to topic." << endl;
                }

            } else if (!strncmp(buff , "unsubscribe" , 11)){
                
                string aux(buff);
                stringstream a(aux);
                string cmd , topic;
                a >> cmd >> topic;

                if (topic.length() > 51) {
                    printf("Unsubscribe usage: unsubscribe topic\n");
                } else {
                    string s_to_send =cmd + " " + topic;
                    int len = cmd.length() + topic.length() + 1;
                    send(sock, s_to_send.c_str() , len, 0);
                    cout << "Unsubscribed from topic." << endl;
                }
            }
        }
    }
    close(sock);
    return 0;
}