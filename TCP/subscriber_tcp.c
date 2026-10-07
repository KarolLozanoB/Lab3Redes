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

    /*  socket(): Creación del socket TCP del cliente. */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP del Broker (Localhost)

    /*  connect(): Establece la conexión con el Broker. */
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error de conexión");
        return 1;
    }

    printf("Ingrese el tema al que desea suscribirse: ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = 0; // Remueve el salto de línea generado por fgets

    // Informar al broker nuestra suscripción
    snprintf(buffer, sizeof(buffer), "SUB|%s", topic);
    
    /* send(): Envía la cadena de suscripción al servidor. */
    send(sock, buffer, strlen(buffer), 0);
    printf("Suscrito a [%s]. Esperando mensajes...\n\n", topic);

    int bytes_read;
    /*  recv(): Se ejecuta en un bucle infinito. Se bloquea (pausa) hasta que lleguen datos.
       Si el broker se desconecta, recv devuelve 0 y el ciclo se rompe. */
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Recibido: %s\n", buffer);
    }

    printf("El broker ha cerrado la conexión.\n");
    close(sock);
    return 0;
}