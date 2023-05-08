# Aplicatie client-server TCP si UDP pentru gestionarea mesajelor
## Popescu Mihai-Costel 324CD , AC-CTI


### Server
* Se creaza 2 sockets unul TCP si unul UDP, apoi se incepe rularea 
server-ului, iar la primirea comenzii ```exit``` serverul se va oprii 
iar toti sockets deschisi se vor inchide.
* Functii regasite in ```server.cpp``` :
    * ``` init_socket_TCP ``` => initializeaza socket-ul TCP;
    * ``` init_socket_UDP ``` => initializeaza socket-ul UDP;
    * ``` send_to_client ``` => se trimit mesajele primite de tip UDP 
    mai departe catre clientii care sunt online si sunt ambonati la 
    topicul respectiv, iar pentru cei care sunt offline si sunt abonati 
    la topicul curent si au sf-ul (store-and-forward) setat pe 1 se 
    stocheaza mesajele pentru a le trimite cand se reconecteaza.
    * ``` sending_lost_msg ``` => se timit mesajele stocate cat timp 
    utilizatorul a fost offline
    * ``` run_sv ``` => se primesc informatii, iar in functie de tipul 
    socket-ului se realizeaza actiunile necesare.

### Subscriber
* Se creaza socket-ul TCP, apoi se incepe rularea subscriber-ului.
* Functii regasite in ```subscriber.cpp``` :
    * ``` init_socket_TCP ``` => initializeaza socket-ul TCP;
    * ``` print_INT , print_SHORT_REAL , print_FLOAT , print_STRING ```
     => se interpreteaza continutul mesajului redirectionat de catre 
    server (cel de tip UDP) si se printeza.
    * ``` run_sub ``` => se primesc mesaje de la server si se primesc 
    comenzi precum ```exit``` unde clientul se deconecteaza de la server 
    sau ``` subscribe , unsubscribe ``` unde se trimit informatii catre 
    server legate de topicurile la care clientul de aboneaza sau se 
    dezaboneaza.