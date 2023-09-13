#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#include <cmath>

void fireman(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
} // Referenced this function from the in-lecture program

std::string CalculateFBarBinary(double fBar);                                   // Returns a string representing Fbar(x) in binary
void AssembleCodeWord(char word[], int length, const std::string binaryString); // Assembles the code word based on the provided length

// This code is reference from the in-lecture program
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    signal(SIGCHLD, fireman);
    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket";
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR on binding";
        exit(1);
    }
    listen(sockfd, 10);
    clilen = sizeof(cli_addr);
    while (true)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
        if (fork() == 0)
        {
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept";
                exit(1);
            }

            // 1. First, receive the symbol's information from the client program.
            double symbolProb, symbolFx;

            n = read(newsockfd, &symbolProb, sizeof(double)); // receiving the probability
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }

            n = read(newsockfd, &symbolFx, sizeof(double)); // receiving the value of f(x)
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }

            // 2. Next, use the Shannon-Fannon-Elias encoding algorithm to generate the binary code of the symbol.
            double symbolFBar = (symbolFx - symbolProb) + symbolProb / 2;
            std::string fBarInBinary = CalculateFBarBinary(symbolFBar);
            int codeLength = ceil(log2(1 / symbolProb) + 1);
            char *codeWord = new char[codeLength];
            AssembleCodeWord(codeWord, codeLength, fBarInBinary);

            // 3. Finally, return the binary code to the client program using sockets.
            int sMessage = strlen(codeWord);
            n = write(newsockfd, &sMessage, sizeof(int)); // preparing the client to receive a char array by first sending the length of the array
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }
            n = write(newsockfd, codeWord, sMessage); // send the binary code (char array) to the client
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }
            close(newsockfd);
            delete[] codeWord;
            _exit(0);
        }
    }
    close(sockfd);
    return 0;
}

// Returns a string representing Fbar(x) in binary
std::string CalculateFBarBinary(double fBar)
{
    std::string fBarBinary{};
    while (fBar)
    {
        int bit = fBar * 2;
        fBar = fmod(fBar * 2, 1);
        fBarBinary.append(std::to_string(bit));
    }

    return fBarBinary;
}

// Assembles the code word based on the provided length
void AssembleCodeWord(char word[], int length, const std::string binaryString)
{
    for (int i = 0; i < length; i++)
    {
        word[i] = binaryString.at(i);
    }
}