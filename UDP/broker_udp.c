#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PUERTO 5000
#define MAX_SUBS 50

/* Tabla de suscriptores: socket y partido que siguen */
struct sockaddr_in subs[MAX_SUBS];
char partidos[MAX_SUBS][50];
int num_subs = 0;

int main() {
  
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    /* Asociar el socket al puerto 5000 en todas las interfaces */
    struct sockaddr_in dir_broker;
    memset(&dir_broker, 0, sizeof(dir_broker));
    dir_broker.sin_family = AF_INET;
    dir_broker.sin_addr.s_addr = INADDR_ANY;
    dir_broker.sin_port = htons(PUERTO);
    if (bind(sock, (struct sockaddr *)&dir_broker, sizeof(dir_broker)) < 0) {
        perror("bind");
        return 1;
    }
    printf("Broker UDP escuchando en el puerto %d\n", PUERTO);

    char buffer[1024];
    char partido[50];
    struct sockaddr_in cliente;
    socklen_t len;

    while (1) {
        /* Recibir un datagrama y la direccion de quien lo envio */
        len = sizeof(cliente);
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&cliente, &len);
        if (n < 0) continue;
        buffer[n] = '\0';

        if (sscanf(buffer, "SUB %49s", partido) == 1) {
            /* Mensaje de suscripcion: guardar al suscriptor */
            if (num_subs < MAX_SUBS) {
                subs[num_subs] = cliente;
                strcpy(partidos[num_subs], partido);
                num_subs++;
                printf("Nuevo suscriptor a %s\n", partido);
            }
        } else if (sscanf(buffer, "PUB %49s", partido) == 1) {
            /* Mensaje de un publicador: reenviarlo a los suscriptores del partido */
            for (int i = 0; i < num_subs; i++) {
                if (strcmp(partidos[i], partido) == 0) {
                    sendto(sock, buffer, n, 0,
                           (struct sockaddr *)&subs[i], sizeof(subs[i]));
                }
            }
            printf("Reenviado: %s\n", buffer);
        }
    }
    return 0;
}