#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define DEFAULT_PORT 8080
#define DEFAULT_ROOT_DIR "./"

void send_file(int client_socket, const char* file_path) {
    char buf[BUF_SIZE];
    int bytes_read;
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        // If file not found, send 404 Not Found error
        char response[BUF_SIZE];
        sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nFile not found", 14);
        write(client_socket, response, strlen(response));
    } else {
        // If file found, send 200 OK response and the file contents
        char response[BUF_SIZE];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", 0);
        write(client_socket, response, strlen(response));
        while ((bytes_read = fread(buf, 1, BUF_SIZE, file)) > 0) {
            write(client_socket, buf, bytes_read);
        }
        fclose(file);
    }
}

void handle_request(int client_socket, const char* root_dir) {
    char request[BUF_SIZE];
    char method[BUF_SIZE];
    char path[BUF_SIZE];
    char file_path[BUF_SIZE];
    int path_len;
    int bytes_read = read(client_socket, request, BUF_SIZE);
    if (bytes_read < 0) {
        perror("read");
        return;
    }
    sscanf(request, "%s %s", method, path);
    // Only handle GET requests
    if (strcasecmp(method, "GET") != 0) {
        char response[BUF_SIZE];
        sprintf(response, "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nMethod not implemented", 22);
        write(client_socket, response, strlen(response));
        return;
    }
    // Construct the full file path from the root directory and the request path
    path_len = strlen(path);
    if (path_len > BUF_SIZE - strlen(root_dir) - 1) {
        // Path too long, send 400 Bad Request error
        char response[BUF_SIZE];
        sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad request", 12);
        write(client_socket, response, strlen(response));
        return;
    }
    strcpy(file_path, root_dir);
    if (path_len == 0 || path[0] != '/') {
        strcat(file_path, "/");
    }
    strcat(file_path, path);
    // Send the file
    send_file(client_socket, file_path);
}

int main(int argc, char* argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    int port = DEFAULT_PORT;
    const char* root_dir = DEFAULT_ROOT_DIR;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (argc > 2) {
            root_dir = argv[2];
        }
    }
   // Create the server socket
server_socket = socket(AF_INET, SOCK_STREAM, 0);
if (server_socket < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
}

// Bind the server socket to the port
memset(&server_address, 0, sizeof(server_address));
server_address.sin_family = AF_INET;
server_address.sin_addr.s_addr = htonl(INADDR_ANY);
server_address.sin_port = htons(port);
if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
}

// Listen for incoming connections
if (listen(server_socket, SOMAXCONN) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
}

