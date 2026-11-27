#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP    "127.0.0.1"
#define PUERTO       6666
#define CODIGO_LEN   6
#define BUFFER_SIZE  256

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PUERTO);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Direccion IP invalida\n");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    char codigo[BUFFER_SIZE];

    printf("Codigo del episodio (ej. S01E01): ");
    fflush(stdout);

    if (fgets(codigo, sizeof(codigo), stdin) == NULL) {
        fprintf(stderr, "No se pudo leer el codigo\n");
        close(sock);
        return 1;
    }

   
    codigo[strcspn(codigo, "\n")] = '\0';


    if (strlen(codigo) != CODIGO_LEN) {
        fprintf(stderr, "Error: el codigo debe tener exactamente %d caracteres\n",
                CODIGO_LEN);
        close(sock);
        return 1;
    }

   
    if (send(sock, codigo, CODIGO_LEN, 0) < 0) {
        perror("send");
        close(sock);
        return 1;
    }

   
    char respuesta[BUFFER_SIZE];
    size_t total = 0;
    ssize_t n;

    
    while (total < sizeof(respuesta) - 1 &&
           (n = recv(sock, respuesta + total, sizeof(respuesta) - 1 - total, 0)) > 0) {
        total += (size_t)n;
    }

    respuesta[total] = '\0';   
    printf("%s", respuesta);

    /* 4. Cerrar el socket */
    close(sock);
    return 0;
}
