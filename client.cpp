#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <algorithm>
#include <vector>

struct Symbol
{
    char symbolLetter{}; // A
    int frequency{};     // the number of occurences from the input message
    double prob{};       // probability = frequency / total number of entries
    double fX{};         // cumalative probability F(x)
    char *codeWord;
    const char *portNum;
    const char *host;
};

bool compareByFrequency(const Symbol &a, const Symbol &b) { return a.frequency > b.frequency; }      // Comparison to be used when sorting by increasing frequency
bool compareByLetter(const Symbol &a, const Symbol &b) { return a.symbolLetter < b.symbolLetter; }   // Comparison to be used when sorting by lexicographic order
void BuildSymbolContainer(std::vector<Symbol> &symbolContainer);                                     // Reads input from STDIN, determines the alphabet & frequence of each symbol, and sorts based on frequency and lexicographic order
void CalculateSymbolProb(std::vector<Symbol> &symbolContainer);                                      // Calculates the probability for each symbol in the container
void CalculateSymbolfX(std::vector<Symbol> &symbolContainer);                                        // Calculates the value of f(x) for each symbol in the container
int CalculateTotalEntries(const std::vector<Symbol> &symbolContainer);                               // Returns the sum of each symbol's frequency
void *SendInfoToServer(void *void_ptr);                                                              // Creates socket to communicate with the server and sends symbol information
void AssignPortAndHost(std::vector<Symbol> &symbolContainer, const char *portNum, const char *host); // Assigns port number and host name for each symbol

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        std::cerr << "usage " << argv[0] << "hostname port\n";
        exit(0);
    }

    std::vector<Symbol> symbolContainer;   // vector used to store each symbol and info
    BuildSymbolContainer(symbolContainer); // determines the alphabet of the message and the frequency of each symbol in the alphabet, sorting the alphabet in decreasing order based on the frequency
    CalculateSymbolProb(symbolContainer);
    CalculateSymbolfX(symbolContainer);
    AssignPortAndHost(symbolContainer, argv[2], argv[1]); // Assigns port number and host for each symbol to the values provided in the command line arguments

    const int NTHREADS = symbolContainer.size(); // NTHREADS is determine by the number of symbols
    pthread_t tid[NTHREADS];

    for (int i = 0; i < NTHREADS; i++)
    {
        if (pthread_create(&tid[i], nullptr, SendInfoToServer, &symbolContainer.at(i)) != 0)
        { // 1. Create a socket to communicate with the server program.
            std::cerr << "Cannot create thread" << std::endl;
        }
    }

    for (int i = 0; i < NTHREADS; i++)
    {
        pthread_join(tid[i], nullptr); // join threads
    }

    std::cout << "SHANNON-FANO-ELIAS Codes:" << std::endl
              << std::endl;
    for (int i = 0; i < NTHREADS; i++)
    {
        std::cout << "Symbol " << symbolContainer.at(i).symbolLetter << ", Code: " << symbolContainer.at(i).codeWord << std::endl;
    }
    return 0;
}

// Reads input from STDIN, determines the alphabet & frequence of each symbol, and sorts based on frequency and lexicographic order
void BuildSymbolContainer(std::vector<Symbol> &symbolContainer)
{
    std::string message{};
    std::getline(std::cin, message);

    for (size_t i = 0; i < message.length(); i++)
    {
        bool letterExists = false;
        int letterPosition{};
        for (int j = 0; j < symbolContainer.size(); j++)
        {
            if (message.at(i) == symbolContainer.at(j).symbolLetter)
            {
                letterExists = true;
                letterPosition = j;
            }
        }
        if (letterExists)
        {
            symbolContainer.at(letterPosition).frequency += 1;
        }
        else
        {
            struct Symbol newSymbol;
            newSymbol.symbolLetter = message.at(i);
            newSymbol.frequency = 1;
            symbolContainer.push_back(newSymbol);
        }
    }

    std::sort(symbolContainer.begin(), symbolContainer.end(), compareByFrequency);
    std::sort(symbolContainer.begin(), symbolContainer.end(), compareByLetter);
}

// Calculates the probability for each symbol in the container
void CalculateSymbolProb(std::vector<Symbol> &symbolContainer)
{
    int totalEntries = CalculateTotalEntries(symbolContainer);
    for (size_t i = 0; i < symbolContainer.size(); i++)
    {
        symbolContainer.at(i).prob = static_cast<double>(symbolContainer.at(i).frequency) / totalEntries;
    }
}

// Calculates the value of f(x) for each symbol in the container
void CalculateSymbolfX(std::vector<Symbol> &symbolContainer)
{
    double sumProb{};
    for (size_t i = 0; i < symbolContainer.size(); i++)
    {
        sumProb += symbolContainer.at(i).prob;
        symbolContainer.at(i).fX = sumProb;
    }
}

// Returns the sum of each symbol's frequency
int CalculateTotalEntries(const std::vector<Symbol> &symbolContainer)
{
    int totalEntries{};
    for (size_t i = 0; i < symbolContainer.size(); i++)
    {
        totalEntries += symbolContainer.at(i).frequency;
    }

    return totalEntries;
}

// Assigns port number and host name for each symbol
void AssignPortAndHost(std::vector<Symbol> &symbolContainer, const char *portNum, const char *host)
{
    for (size_t i = 0; i < symbolContainer.size(); i++)
    {
        symbolContainer.at(i).portNum = portNum;
        symbolContainer.at(i).host = host;
    }
}

// Creates socket to communicate with the server and sends symbol information
// This function contains code that has been reference from the in lecture program
void *SendInfoToServer(void *void_ptr)
{
    Symbol *arg = (Symbol *)void_ptr; // cast the void pointer to a pointer of type Symbol
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = atoi(arg->portNum);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        std::cerr << "ERROR opening socket";
    server = gethostbyname(arg->host);
    if (server == NULL)
    {
        std::cerr << "ERROR, no such host\n";
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR connecting";
        exit(1);
    }

    // 2. Send the symbol's information to the server program using sockets.
    n = write(sockfd, &arg->prob, sizeof(double)); // sending the symbol's probability to the server
    if (n < 0)
    {
        std::cerr << "ERROR writing to socket";
        exit(1);
    }
    n = write(sockfd, &arg->fX, sizeof(double)); // sending the symbol's value of f(x) to the server
    if (n < 0)
    {
        std::cerr << "ERROR writing to socket";
        exit(1);
    }

    // 3. Wait for the binary code from the server program.
    int size;                             // used to store the size of the char array
    n = read(sockfd, &size, sizeof(int)); // receiving the size
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket";
        exit(1);
    }
    arg->codeWord = new char[size + 1]; // dynamic char array  that will store the received code word
    bzero(arg->codeWord, size + 1);

    // 4. Write the received information into a memory location accessible by the main thread.
    n = read(sockfd, arg->codeWord, size); // receiving the char array
    close(sockfd);

    return nullptr;
}