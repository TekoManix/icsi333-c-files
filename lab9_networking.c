#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define PORT 8080

void server_mode() {
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	char buffer[BUFFER_SIZE];
	
	// Create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Socket creation failed");
		exit(1);
	}

	// Set socket options to reuse address
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	
	// Bind socket
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		close(server_fd);
		exit(1);
	}
	
	// Listen for connections
	if (listen(server_fd, 1) < 0) {
		perror("isten failed");
		close(server_fd);
		exit(1);
	}
	
	printf("Server listening on 127.0.0.1%d\n", PORT);
	printf("Run client with: ./network 127.0.0.1 %d\n", PORT);
	
	// Accept connection
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) {
		perror("Accept failed");
		close(server_fd);
		exit(1);
	}
	
	printf("Client connected!\n");
	
	// Communication loop
	while (1) {
		// Receive message
		memset(buffer, 0, BUFFER_SIZE);
		int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_received <= 0) {
			printf("Client disconnected\n");
			break;
		}
		printf("Received: %s\n", buffer);
		
		// Send pong
		const char *message = "pong";
		send(client_fd, message, strlen(message), 0);
		printf("Send: %s\n", message);
	}
	
	close(client_fd);
	close(server_fd);
}

void client_mode(const char *address, int port) {
	int sock_fd;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	
	// Create socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("Socket creation failed");
		exit(1);
	}
	
	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	
	if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
		perror("Invalid address");
		close(sock_fd);
		exit(1);
	}
	
	// Connect to server
	if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		close(sock_fd);
		exit(1);
	}
	
	printf("Connected to server at %s:%d\n", address, port);
	
	// Communication loop
	while (1) {
		// Send ping
		const char *message = "ping";
		send(sock_fd, message, strlen(message), 0);
		printf("Sent: %s\n", message);
		
		// Receive message
		memset(buffer, 0, BUFFER_SIZE);
		int bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_received <= 0) {
			printf("Server disconnected\n");
			break;
		}
		printf("Received: %s\n", buffer);
		
		sleep(1);
	}
	
	close(sock_fd);
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		// Server mode
		printf("Running in SERVER mode\n");
		server_mode();
	} else if (argc == 3) {
		// Client mode
		printf("Running in CLIENT mode\n");
		const char *address = argv[1];
		int port = atoi(argv[2]);
		client_mode(address, port);
	} else {
		printf("Usage:\n");
		printf("Server mode: %s\n", argv[0]);
		printf("Client mode: %s <address> <port>\n", argv[0]);
		return 1;
	}
	
	return 0;
}