#include <iostream>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

/*
 * Globalna premenna pre socket, potrebna pri spravnej komunikacii
 * so serverom a pri c-c handlingu
*/
int clientSocket;

/*
 * Ziskanie riadku zo stdin, neskor pouzite ako sprava.
*/
std::string getLineFromStdin() {
    std::string line;
    std::getline(std::cin, line, '\n');
    return line;
}

/*
 * Kontrola spravneho poctu argumentov v prikazovom riadku.
*/

void numberOfCommandLineArguments(int argc) {
    if (argc != 7) {
        perror("Nespravny pocet argumentov\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Kontrola -h argumentu a adresy
*/

void correctHost(char **argv) {
    if (strcmp(argv[1], "-h")) {
        perror("Wrong host option, fuck you\n");
        exit(EXIT_FAILURE);
    }
}

hostent *correctHostAddress(char **argv) {
    struct hostent *server = gethostbyname(argv[2]);
    if (server == NULL) {
        perror("Invalid host, fuck you");
        exit(EXIT_FAILURE);
    }
    return server;
}

/*
 * Kontrola -p argumentu a spravneho portu
*/

void correctPort(char **argv) {
    if (strcmp(argv[3], "-p")) {
        perror("Wrong port option\n");
        exit(EXIT_FAILURE);
    }
    if (std::stoi(argv[4]) < 0 || std::stoi(argv[4]) > 65535) {
        perror("Invalid port\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Kontrola -m argumentu a spravneho portu
*/

void correctProtocol(char **argv) {
    if (strcmp(argv[5], "-m")) {
        perror("Wrong protocol option\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[6], "tcp") && strcmp(argv[6], "udp")) {
        perror("Unkown protocol\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Pomocna funkcia pre kontrolu celeho prikazoveho riadku
*/

void correctArguments(int argc, char **argv) {
    numberOfCommandLineArguments(argc);
    correctHost(argv);
    correctPort(argv);
    correctProtocol(argv);
}

/*
 * Vytvorenie a kontrola vytvorenia socketu pre UDP
*/

int createSocketUDP() {
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        perror("Error with creating a socket\n");
        exit(EXIT_FAILURE);
    }
    return clientSocket;
}

/*
 * Vytvorenie a kontrola vytvorenia socketu pre UDP
*/

int createSocketTCP() {
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("Error with creating a socket\n");
        exit(EXIT_FAILURE);
    }
    return clientSocket;
}

/*
 * Vytvorenie specifickej spravy, ktora sa posiela na server
*/

char *createMessage(const char *message) {
    int length = strlen(message);
    char *newMessage = new char[length + 3];
    if (newMessage == NULL) {
        perror("Error at memory allocation\n");
        exit(99);
    }
    newMessage[0] = '\x00';
    newMessage[1] = length;
    for (int i = 0; i < length; ++i) {
        newMessage[i + 2] = message[i];
    }
    return newMessage;
}

/*
 * Zmena c++ stringu na pole charakterov potrebne pre posielanie socketu
*/

char *createCharArray() {
    std::string msg = getLineFromStdin();
    char *message = new char[msg.length() + 1];
    strcpy(message, msg.c_str());
    return message;
}

/*
 * Dlzka spravy
*/

int getMessageLength(char *message) {
    return strlen(message) + 1;
}

/*
 * Poslanie UDP socketu
*/

void sendSocketUDP(int clientSocket, char *message, int messageLength, struct sockaddr_in serverAddress) {
    if (sendto(clientSocket, createMessage(message), messageLength + 2, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Chyba v posielani socketu\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Vypis chybnej hlasky
*/

void printResultERR() {
    std::cout << "ERR:Could not parse the message" << "\n";
}

/*
 * Vypis spravnej hlasky
*/

void printResultOK(char *buffer) {
    std::cout << "OK:";
    for (long i = 3; i < buffer[2] + 3; ++i) {
        std::cout << buffer[i];
    }
    std::cout << "\n";
}

/*
 * Pomocna funkcia pre vypis
*/

void printResult(char *buffer) {
    if (buffer[1] == '\x00') {
        printResultOK(buffer);
    }
    else {
        printResultERR();
    }
}

/*
 * Ziskanie spravy od serveru
*/

void receiveSocketUDP(int clientSocket, char *buffer, struct sockaddr_in serverAddress, socklen_t serAddrLen) {
    int bytes_rx;
    if ((bytes_rx = recvfrom(clientSocket, buffer, 255, 0, (struct sockaddr*)&serverAddress, &serAddrLen)) < 0) {
        perror("Chyba pri ziskani socketu\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Pripojenie na server pri TCP
*/

void connectSocketTCP(sockaddr_in serverAddress) {
    if (connect(clientSocket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror("Chyba pri TCP pripojeni\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Zaslanie spravy na server pri TCP
*/

void sendMessageTCP(char *message) {
    int bytes_tx;
    if ((bytes_tx = send(clientSocket, message, strlen(message), 0)) < 0) {
        perror("Chyba pri posielani TCP spravy\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Zisk spravy od servera pri TCP
*/

void receiveSocketTCP(char *buffer) {
    int bytes_rx;
    if ((bytes_rx = recv(clientSocket, buffer, 255, 0)) < 0) {
        perror("Chyba pri ziskani socketu od serveru\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Uzatvorenie socketu
*/

void closeSocket(int clientSocket) {
    if (close(clientSocket) == -1) {
        perror("Failed to close the socket\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Odchytenie c-c signalu
*/

void catchSignal(int) {
    closeSocket(clientSocket);
    exit(0);
}

/*
 * Kontrola, ci prva sprava zaslana na server pri TCP je "HELLO"
*/

bool isHelloFirst(char *message) {
    if (!strcmp(message, "HELLO\n")) {
        return true;
    }
    else {
        
        return false;
    }
}

/*
 * Kontrola, ci sprava zaslana na server pri TCP je "BYE"
*/

bool isMessageBye(char *message) {
    if (!strcmp(message, "BYE\n")) {
        return true;
    }
    else {
        
        return false;
    }
}

/*
 * Uzatvorenie UDP socketu
*/

void closeUDPSocket() {
    shutdown(clientSocket, SHUT_RD);
    shutdown(clientSocket, SHUT_WR);
    shutdown(clientSocket, SHUT_RDWR);
    closeSocket(clientSocket);
    exit(0);
}

/*
 * Hlavna funkcia pri komunikacii so serverom
 * Ak bol zadany udp protokol, zacne sa komunikacia so serverom
 * a su volane potrebne funkcie v cykle
 * To iste plati v pripade, ze bol zadany tcp protokol
*/

void startCommunication(sockaddr_in serverAddress, char **argv, socklen_t serAddrLen) {
    char buffer[255] = { 0 };
    if (!strcmp(argv[6], "udp")) {
        clientSocket = createSocketUDP();
        while (true) {
            char *message = createCharArray();
            if (strcmp(message, "") == 0) {
                closeUDPSocket();
            }
            int messageLength = getMessageLength(message);
            sendSocketUDP(clientSocket, message, messageLength, serverAddress);
            receiveSocketUDP(clientSocket, buffer, serverAddress, serAddrLen);
            printResult(buffer);
            std::fill(std::begin(buffer), std::end(buffer), 0);
        }
    }
    else {
        clientSocket = createSocketTCP();
        connectSocketTCP(serverAddress);
        char message[100];
        bool firstMessage = true;
        while (true) {
            if (fgets(message, 100, stdin) == NULL) {
                exit(1);
            }
            if (firstMessage) {
                if (!isHelloFirst(message)) {
                    closeUDPSocket();
                }
                firstMessage = false;
            }
            sendMessageTCP(message);
            receiveSocketTCP(buffer);
            for (size_t i = 0; i < strlen(buffer); ++i) {
                std::cout << buffer[i];
            }
            if (isMessageBye(message)) {
                closeUDPSocket();
            }
            std::fill(std::begin(buffer), std::end(buffer), 0);
        }
    }
}


int main(int argc, char **argv) {
    signal(SIGINT, catchSignal);
    correctArguments(argc, argv);
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(std::stoi(argv[4]));
    serverAddress.sin_addr.s_addr = inet_addr(argv[2]);
    socklen_t serAddrLen = sizeof(serverAddress);
    startCommunication(serverAddress, argv, serAddrLen);
    return 0;
}