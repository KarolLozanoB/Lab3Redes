#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PUERTO 5000

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <ip_broker> <partido> [cantidad]\n", argv[0]);
        return 1;
    }
    char *partido = argv[2];
    int cantidad = (argc > 3) ? atoi(argv[3]) : 10;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    /* Direccion del broker */
    struct sockaddr_in broker;
    memset(&broker, 0, sizeof(broker));
    broker.sin_family = AF_INET;
    broker.sin_port = htons(PUERTO);
    inet_pton(AF_INET, argv[1], &broker.sin_addr);

    char *eventos[] = {"Gol del equipo local", "Tarjeta amarilla",
                       "Cambio de jugador", "Gol del equipo visitante",
                       "Tarjeta roja"};
    char mensaje[256];

    for (int i = 1; i <= cantidad; i++) {
        /* Formato: PUB partido numero evento */
        snprintf(mensaje, sizeof(mensaje), "PUB %s %d %s",
                 partido, i, eventos[(i - 1) % 5]);
        sendto(sock, mensaje, strlen(mensaje), 0,
               (struct sockaddr *)&broker, sizeof(broker));
        printf("Enviado: %s\n", mensaje);
        usleep(500000);   
    }

    close(sock);
    return 0;
}