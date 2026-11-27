#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bd.h"
#include "formato.h"

#define PUERTO 6666
#define CODIGO_LEN 6
#define BUFFER_SIZE 256

/* Para recoger todos los procesos hijos terminados */
void hijo_terminado(int sig) {
    (void)sig;

    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }

    errno = saved_errno;
}

/* Recibe exactamente n bytes */
int leer_exactamente(int fd, char *buffer, size_t n) {
    size_t total = 0;

    while (total < n) {
        ssize_t bytes = read(fd, buffer + total, n - total);

        if (bytes == 0) {
            return -1;
        }

        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        total += (size_t)bytes;
    }

    return 0;
}

/* Esta es para escribir todos los bytes utilizando el socket */
int enviar_completo(int fd, const char *buffer, size_t n) {
    size_t total = 0;

    while (total < n) {
        ssize_t bytes = send(
            fd,
            buffer + total,
            n - total,
            MSG_NOSIGNAL
        );

        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes == 0) {
            return -1;
        }

        total += (size_t)bytes;
    }

    return 0;
}

/* Escribe todos los bytes en pipe */
int escribir_pipe(int fd, const char *buffer, size_t n) {
    size_t total = 0;

    while (total < n) {
        ssize_t bytes = write(fd, buffer + total, n - total);

        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes == 0) {
            return -1;
        }

        total += (size_t)bytes;
    }

    return 0;
}

/* Lee una cadena terminada en nulo desde pipe */
int leer_ip_pipe(int fd, char *buffer, size_t tam) {
    size_t total = 0;

    while (total < tam - 1) {
        ssize_t bytes = read(fd, buffer + total, 1);

        if (bytes == 0) {
            return -1;
        }

        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (buffer[total] == '\0') {
            return 0;
        }

        total++;
    }

    buffer[tam - 1] = '\0';

    return -1;
}

/* Atiende la solicitud de un cliente */
void atender_cliente(int client_fd) {

    char codigo[CODIGO_LEN + 1] = {0};

    /* Recibir los seis caracteres */
    if (leer_exactamente(client_fd, codigo, CODIGO_LEN) < 0) {
        close(client_fd);
        _exit(1);
    }

    codigo[CODIGO_LEN] = '\0';

    /* Consulta y formatea la respuesta */
    char buffer[BUFFER_SIZE];

    int len = formato_respuesta(
        codigo,
        buffer,
        sizeof(buffer)
    );

    if (len < 0) {
        close(client_fd);
        _exit(1);
    }

    /* Para evitar enviar más bytes de los almacenados */
    size_t bytes_a_enviar = (size_t)len;

    if (bytes_a_enviar >= sizeof(buffer)) {
        bytes_a_enviar = sizeof(buffer) - 1;
    }

    /* Enviar la respuesta */
    if (enviar_completo(
            client_fd,
            buffer,
            bytes_a_enviar
        ) < 0) {

        perror("Error enviando respuesta");
    }

    close(client_fd);

    _exit(0);
}

int main(int argc, char **argv) {

    int test_run = (
        argc > 1 &&
        strcmp(argv[1], "--test-run") == 0
    );

    /* Inicializar la base de datos */
    bd_init();

    printf("bd cargada: %d episodios\n", bd_cantidad());

    /* Crear el socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Reutilizar la dirección */
    int opt = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) < 0) {

        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    /* Configurar la dirección del server */
    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PUERTO);

    /* Para asociar el socket al puerto */
    if (bind(
            server_fd,
            (struct sockaddr *)&address,
            sizeof(address)
        ) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    /* Poner el server a escuchar */
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Servidor escuchando en el puerto %d...\n", PUERTO);
    fflush(stdout);

    /* Ignorar SIGINT */
    struct sigaction sa_int = {0};

    sa_int.sa_handler = SIG_IGN;
    sigemptyset(&sa_int.sa_mask);

    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
        close(server_fd);
        return 1;
    }

    /* Configuracion de SIGCHLD */
    struct sigaction sa_child = {0};

    sa_child.sa_handler = hijo_terminado;
    sigemptyset(&sa_child.sa_mask);
    sa_child.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa_child, NULL) < 0) {
        perror("sigaction SIGCHLD");
        close(server_fd);
        return 1;
    }

    /* Test */
    if (test_run) {
        printf("--test-run OK\n");
        close(server_fd);
        return 0;
    }

    /* Bucle principal del server */
    while (1) {

        struct sockaddr_in client_addr = {0};
        socklen_t client_len = sizeof(client_addr);

        /* Aceptar conexión */
        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (client_fd < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            continue;
        }

        /* Obtener la dirección IP */
        char ip_cliente[INET_ADDRSTRLEN] = {0};

        if (inet_ntop(
                AF_INET,
                &client_addr.sin_addr,
                ip_cliente,
                sizeof(ip_cliente)
            ) == NULL) {

            strcpy(ip_cliente, "desconocida");
        }

        /* Crear pipe para este cliente */
        int pipe_fd[2];

        if (pipe(pipe_fd) < 0) {
            perror("pipe");
            close(client_fd);
            continue;
        }

        /* Crear un proceso hijo */
        pid_t pid = fork();

        if (pid == 0) {

            /* HIJO */

            close(server_fd);
            close(pipe_fd[0]);

            /* Aqui restauramos SIGCHLD por si el hijo crea descendientes */
            struct sigaction sa_default = {0};
            sa_default.sa_handler = SIG_DFL;
            sigemptyset(&sa_default.sa_mask);
            sigaction(SIGCHLD, &sa_default, NULL);

            /* Enviar IP al padre */
            if (escribir_pipe(
                    pipe_fd[1],
                    ip_cliente,
                    strlen(ip_cliente) + 1
                ) < 0) {

                close(pipe_fd[1]);
                close(client_fd);
                _exit(1);
            }

            close(pipe_fd[1]);

            /* Atender cliente */
            atender_cliente(client_fd);

        }
        else if (pid > 0) {

            /* PADRE */

            close(client_fd);
            close(pipe_fd[1]);

            /* Leer IP desde pipe */
            char ip_buffer[INET_ADDRSTRLEN] = {0};

            if (leer_ip_pipe(
                    pipe_fd[0],
                    ip_buffer,
                    sizeof(ip_buffer)
                ) == 0) {

                printf(
                    "Conexion atendida por el hijo PID %d - IP: %s\n",
                    pid,
                    ip_buffer
                );

                fflush(stdout);

            } else {

                fprintf(
                    stderr,
                    "Error recibiendo IP del hijo PID %d\n",
                    pid
                );
            }

            close(pipe_fd[0]);

        }
        else {

            /* ERROR AL CREAR PROCESO */

            perror("fork");

            close(client_fd);
            close(pipe_fd[0]);
            close(pipe_fd[1]);
        }
    }

    close(server_fd);

    return 0;
}