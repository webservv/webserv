#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    const char* serverIP = "127.0.0.1";  // Replace with the server's IP address
    const int serverPort = 8080;        // Replace with the server's port

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // Set up the server address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to server." << std::endl;
        close(clientSocket);
        return 1;
    }

    // Send a message to the server
    const char* message = "Hello, echo server!";
    send(clientSocket, message, strlen(message), 0);

    // Receive and print the echoed message from the server
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::cout << "Received: " << buffer << std::endl;
    } else {
        std::cerr << "Error receiving data from server." << std::endl;
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
