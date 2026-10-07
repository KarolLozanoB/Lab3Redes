#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PUERTO 5000

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <ip_broker> <partido1> [partido2 ...]\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    /* Direccion del broker */
    struct sockaddr_in broker;
    memset(&broker, 0, sizeof(broker));
    broker.sin_family = AF_INET;
    broker.sin_port = htons(PUERTO);
    inet_pton(AF_INET, argv[1], &broker.sin_addr);

    /* Enviar un mensaje SUB por cada partido */
    char mensaje[100];
    for (int i = 2; i < argc; i++) {
        snprintf(mensaje, sizeof(mensaje), "SUB %s", argv[i]);
        sendto(sock, mensaje, strlen(mensaje), 0,
               (struct sockaddr *)&broker, sizeof(broker));
        printf("Suscrito a %s\n", argv[i]);
    }

    /* Recibir y mostrar las actualizaciones */
    char buffer[1024];
    while (1) {
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
        if (n < 0) continue;
        buffer[n] = '\0';
        printf("%s\n", buffer);
    }
    return 0;
}