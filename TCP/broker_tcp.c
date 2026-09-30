#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <arpa/inet.h>   
#include <sys/select.h>  

#define MAX_CLIENTS 10
#define PORT 8080

// Estructura para administrar la informacion de cada cliente conectado
typedef struct {
    int sock;          // 0 si la ranura esta libre
    char topic[100];   // Tema de interes
    int is_sub;        // 1 si es un suscriptor, 0 si aun no se define
} Client;

int main() {
    int server_sock, new_sock, max_sd;
    struct sockaddr_in server_addr;
    Client clients[MAX_CLIENTS] = {0}; 
    fd_set readfds; // estructura especifica pa sockest en unix

   
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Configuracion de la direccion y puerto del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // htons asegura el orden correcto de bytes para la red
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escucha en cualquier interfaz de red local

    /*bind(): Asocia el socket recien creado con la IP y el puerto definidos. */
    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    /*  listen(): Pone al socket en modo pasivo para que espere conexiones.
       - 15: Es la longitud maxima de la cola de conexiones pendientes. */
    listen(server_sock, 15);
    printf("Broker TCP iniciado. Escuchando en el puerto %d...\n", PORT);

    while (1) {
        // Limpiamos el conjunto de vigilancia y agregamos el socket del servidor
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        max_sd = server_sock;

        // Agregamos tambien los sockets de los clientes ya conectados
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &readfds);
            }
            if (clients[i].sock > max_sd) max_sd = clients[i].sock;
        }

        /* select(): Bloquea el programa hasta que haya actividad en algun socket.
           evisa si hay nuevas conexiones o si un cliente envio datos. */
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // Si la actividad fue en server_sock, significa que hay un NUEVO cliente intentando conectarse
        if (FD_ISSET(server_sock, &readfds)) {
            /* 5. accept(): Acepta la conexion entrante y crea un NUEVO socket dedicado a ese cliente. */
            new_sock = accept(server_sock, NULL, NULL);
            
            // Guardamos el nuevo socket en el primer espacio disponible del arreglo
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = new_sock;
                    clients[i].is_sub = 0;
                    printf("Nuevo cliente conectado en el socket %d.\n", new_sock);
                    break;
                }
            }
        }

        // Revisamos si la actividad fue de un cliente que YA estaba conectado enviando un mensaje
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[1024];
                
                /* recv(): Lee los datos que llegaron al socket del cliente.
                   - sd: El socket del cliente.
                   - buffer: Donde se guardan los datos.
                   - 0: Flags por defecto. */
                int bytes = recv(sd, buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0) {
                    // Si recibe 0 bytes, el cliente cerro la conexion
                    close(sd);
                    clients[i].sock = 0;
                    printf("Cliente desconectado.\n");
                } else {
                    buffer[bytes] = '\0'; // Aseguramos que sea una cadena de texto valida
                    
                    // Logica de enrutamiento del Broker
                    if (strncmp(buffer, "SUB|", 4) == 0) {
                        strcpy(clients[i].topic, buffer + 4);
                        clients[i].is_sub = 1;
                        printf("Socket %d suscrito a: %s\n", sd, clients[i].topic);
                    } 
                    else if (strncmp(buffer, "PUB|", 4) == 0) {
                        char *topic = strtok(buffer + 4, "|");
                        char *msg = strtok(NULL, "");
                        
                        if (topic && msg) {
                            printf("Enrutando mensaje de [%s]: %s\n", topic, msg);
                            
                            // Reenviar a todos los suscriptores interesados
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].is_sub && strcmp(clients[j].topic, topic) == 0) {
                                    /* 7. send(): Envia los datos hacia el suscriptor. */
                                    send(clients[j].sock, msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}