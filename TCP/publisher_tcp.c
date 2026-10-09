#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char topic[100];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error de conexion");
        return 1;
    }

    printf("Conectado al Broker. Se enviaran 10 mensajes automaticos.\n");
    printf("Ingrese el tema para publicar : ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = 0;

    for (int i = 1; i <= 10; i++) {
        snprintf(buffer, sizeof(buffer), "PUB|%s|Mensaje de prueba numero %d\n", topic, i);
        send(sock, buffer, strlen(buffer), 0);
        printf("Enviado: Mensaje %d de 10\n", i);
        sleep(1);
    }

    printf("Todos los mensajes fueron enviados.\n");
    close(sock);
    return 0;
}