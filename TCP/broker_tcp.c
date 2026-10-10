#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // close() 
#include <signal.h>      // signal(), SIGPIPE 
#include <errno.h>       // errno, EINTR 
#include <arpa/inet.h>   // htons(), struct sockaddr_in, INADDR_ANY 
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), send(), recv() 
#include <sys/select.h>  // select(), FD_ZERO, FD_SET, FD_ISSET 

#define MAX_CLIENTS 20
#define PORT 8080
#define BUF_SIZE 1024
#define TOPIC_SIZE 100

typedef struct {
    int sock;
    char topic[TOPIC_SIZE];
    int is_sub;
    char pend[BUF_SIZE];
    int pend_len;
} Client;

void liberar_cliente(Client *c) {
    close(c->sock);
    c->sock = 0;
    c->is_sub = 0;
    c->topic[0] = '\0';
    c->pend_len = 0;
}

int enviar_todo(int sock, const char *data, int len) {
    while (len > 0) {
        int n = send(sock, data, len, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        data += n;
        len -= n;
    }
    return 0;
}

void procesar_mensaje(Client clients[], int i, char *linea) {
    linea[strcspn(linea, "\r")] = '\0';

    if (strncmp(linea, "SUB|", 4) == 0) {
        snprintf(clients[i].topic, TOPIC_SIZE, "%s", linea + 4);
        clients[i].is_sub = 1;
        printf("Socket %d suscrito a: %s\n", clients[i].sock, clients[i].topic);
    }
    else if (strncmp(linea, "PUB|", 4) == 0) {
        char *topic = strtok(linea + 4, "|");
        char *msg = strtok(NULL, "");

        if (!topic || !msg) {
            printf("Mensaje PUB mal formado, se ignora.\n");
            return;
        }
        printf("Enrutando mensaje de [%s]: %s\n", topic, msg);

        char salida[BUF_SIZE + TOPIC_SIZE + 4];
        int len = snprintf(salida, sizeof(salida), "[%s] %s\n", topic, msg);
        if (len >= (int)sizeof(salida)) len = sizeof(salida) - 1;

        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients[j].sock > 0 && clients[j].is_sub &&
                strcmp(clients[j].topic, topic) == 0) {
                if (enviar_todo(clients[j].sock, salida, len) < 0) {
                    printf("No se pudo enviar al socket %d, se desconecta.\n", clients[j].sock);
                    liberar_cliente(&clients[j]);
                }
            }
        }
    }
    else {
        printf("Mensaje desconocido, se ignora: %s\n", linea);
    }
}

int main() {
    int server_sock, new_sock, max_sd;
    struct sockaddr_in server_addr;
    Client clients[MAX_CLIENTS];
    fd_set readfds;

    memset(clients, 0, sizeof(clients));

    signal(SIGPIPE, SIG_IGN);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error en socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error en setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 15) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
    printf("Broker TCP iniciado. Escuchando en el puerto %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        max_sd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &readfds);
                if (clients[i].sock > max_sd) max_sd = clients[i].sock;
            }
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("Error en select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_sock, &readfds)) {
            new_sock = accept(server_sock, NULL, NULL);
            if (new_sock < 0) {
                perror("Error en accept");
            } else {
                int guardado = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sock == 0) {
                        clients[i].sock = new_sock;
                        clients[i].is_sub = 0;
                        clients[i].pend_len = 0;
                        printf("Nuevo cliente conectado en el socket %d.\n", new_sock);
                        guardado = 1;
                        break;
                    }
                }
                if (!guardado) {
                    printf("Broker lleno, se rechaza el socket %d.\n", new_sock);
                    close(new_sock);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd <= 0 || !FD_ISSET(sd, &readfds)) continue;

            int espacio = BUF_SIZE - 1 - clients[i].pend_len;
            int bytes = recv(sd, clients[i].pend + clients[i].pend_len, espacio, 0);

            if (bytes <= 0) {
                printf("Cliente del socket %d desconectado.\n", sd);
                liberar_cliente(&clients[i]);
                continue;
            }

            clients[i].pend_len += bytes;
            clients[i].pend[clients[i].pend_len] = '\0';

            char *inicio = clients[i].pend;
            char *fin;
            while ((fin = strchr(inicio, '\n')) != NULL) {
                *fin = '\0';
                procesar_mensaje(clients, i, inicio);
                if (clients[i].sock == 0) break;
                inicio = fin + 1;
            }
            if (clients[i].sock == 0) continue;

            int resto = clients[i].pend_len - (inicio - clients[i].pend);
            memmove(clients[i].pend, inicio, resto);
            clients[i].pend_len = resto;

            if (clients[i].pend_len >= BUF_SIZE - 1) {
                printf("Mensaje demasiado largo del socket %d, se descarta.\n", sd);
                clients[i].pend_len = 0;
            }
        }
    }
    return 0;
}