#include "../includes/HTTP_server.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/ConfigCheck.hpp"
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <fstream>

#define BUF_SIZE 1024
#define TIMEOUT 20



void HTTP_server::print_request(std::map<std::string, std::string> my_map)
{
    std::map<std::string, std::string>::iterator it;
    switch (color_index) {
        case 0:
            for (it = my_map.begin(); it != my_map.end(); ++it) {
                std::cout << RED << "Key: " << it->first << ", Value: " << it->second << DEF <<std::endl;
            }
            color_index++;
            break;
        case 1:
            for (it = my_map.begin(); it != my_map.end(); ++it) {
                std::cout << YELLOW << "Key: " << it->first << ", Value: " << it->second << DEF <<std::endl;
            }
            color_index++;
            break;
        case 2:
            for (it = my_map.begin(); it != my_map.end(); ++it) {
                std::cout << CYAN << "Key: " << it->first << ", Value: " << it->second << DEF <<std::endl;
            }
            color_index++;
            break;
        case 3:
            for (it = my_map.begin(); it != my_map.end(); ++it) {
                std::cout << CYAN << "Key: " << it->first << ", Value: " << it->second << DEF <<std::endl;
            }
            color_index = 0;
            break;
        default:
            std::cout << "Invalid choice." << std::endl;
            break;
    }
}

/**
 * Constructor of HTTP server, creates an array of client
 *
 * @param filename The name of the file to read.
 * @return The contents of the file as a string.
 */
HTTP_server::HTTP_server(std::string path, char **env): _path(path), _env(env){
    timeoutDuration = TIMEOUT;
    color_index = 0;
}

HTTP_server::~HTTP_server(){}

/**
 * Reads the contents of a file and returns it as a string.
 *
 * @param filename The name of the file to read.
 * @return The contents of the file as a string.
 */
std::string HTTP_server::read_file(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    std::string content;
    char c;
    if (!file)
    {
        std::cerr << "Error opening file " << filename << std::endl;
        return "";
    }
    while (file.get(c))
    {
        content += c;
    }
    file.close();
    return content;
}

/**
 * Tokenizes a line of text and stores the key-value pair in a map.
 *
 * @param request The map to store the key-value pair.
 * @param line_to_tokenize The line of text to tokenize.
 */
void HTTP_server::tokenizing(std::map<std::string, std::string> &request, std::string line_to_tokenize)
{
    std::stringstream tokenize_stream(line_to_tokenize);
    std::string value;
    std::string key;
    key = line_to_tokenize.substr(0, line_to_tokenize.find(":") + 1);
    value = line_to_tokenize.substr(line_to_tokenize.find(":") + 1);
    removeWhitespaces(key);
    removeWhitespaces(value);
    request[key] = value;
}

void HTTP_server::removeWhitespaces(std::string &string)
{
    string.erase(0, string.find_first_not_of(" \t"));
    string.erase(string.find_last_not_of(" \t") + 1);
}

/**
 * Creates the pollfd structures for polling file descriptors.
 *
 * The function initializes the `fds` array with the listening socket file descriptors and sets the events to poll for.
 */
void HTTP_server::create_pollfd_struct(void)
{
    memset(fds, 0, MAX_CLIENTS * sizeof(fds));
    for (int i = 0; i < listening_port_no + MAX_CLIENTS + 1; i++)
    {
        fds[i].fd = FdsClients[i].first;
        fds[i].events = POLLIN | POLLOUT;
    }
}

void HTTP_server::server_port_listening(int i)
{
    if (fds[i].revents & POLLIN)
    {
        for (int j = listening_port_no; j < MAX_CLIENTS + listening_port_no + 1; j++)
        {
            if (fds[j].fd == -1)
            {
                client_len = sizeof(client_addr);
                FdsClients[j].first = accept(FdsClients[i].first, (struct sockaddr *)&client_addr, &client_len);
                activeClientIdx.insert(j);
                if (FdsClients[j].first < 0)
                {
                    perror("Error accepting client connection on etc");
                    exit(EXIT_FAILURE);
                }
                fds[j].fd = FdsClients[j].first;
                FdsClients[j].second.socket = i;
                FdsClients[j].second.lastInteractionTime = time(nullptr);
                if (j == MAX_CLIENTS + listening_port_no)
                    FdsClients[j].second.server_full = true;
                break;
            }
        }
        std::cout << "New client connected\n";
    }
}

/*
 *We can use the timeout with try and reset the poll if the timeout is down because of stability
 *
 */
void HTTP_server::server_conducts_poll()
{
    nfds = MAX_CLIENTS + listening_port_no + 1;
    // timeout = (3 * 60 * 1000);
    timeout = -1;
    res = poll(fds, nfds, timeout);
    if (res < 0)
    {
        perror("Error polling sockets");
        exit(EXIT_FAILURE);
    }
}

std::map<std::string, std::string> HTTP_server::server_mapping_request(int i)
{
    std::string key;
    std::map<std::string, std::string> new_request;
    int new_line_count = 0;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    // Obtain Header Length
    int n = recv(FdsClients[i].first, buf, BUF_SIZE, MSG_DONTWAIT | MSG_PEEK);
    if (n < 0)
    {
        perror("Error receiving data from client1");
        exit(EXIT_FAILURE);
    }
    char *header = std::strstr(buf, "\r\n\r\n");
    size_t headerlength = header - buf + 4;

    // Get lines of Header
    memset(buf, 0, BUF_SIZE);
    n = recv(FdsClients[i].first, buf, headerlength, MSG_DONTWAIT);
    if (n < 0)
    {
        perror("Error receiving data from client2");
        exit(EXIT_FAILURE);
    }

    std::string HTTP_request(buf);
    line = std::strtok(&HTTP_request[0], "\n");
    while (line != NULL)
    {
        std::string strLine(line);
        lines.push_back(strLine);
        line = strtok(NULL, "\n");
    }
    if (!lines.empty())
    {
        new_request["method:"] = std::strtok(&lines.front()[0], " ");
        new_request["location:"] = std::strtok(NULL, " ");
        new_request["HTTP_version:"] = std::strtok(NULL, " ");
    }
    while (!lines.empty())
    {
        if (!lines.empty() &&
            lines.front() == "\n" &&
            new_line_count == 1)
        {
            lines.pop_front();
            break;
        }
        if (!lines.empty() &&
            lines.front() == "\r")
        {
            new_line_count++;
        }
        else
        {
            tokenizing(new_request, lines.front());
        }
        lines.pop_front();
    }
    size_t v;
    if ((v = new_request["Content-Type:"].find("boundary=")) != std::string::npos)
    {
        std::string key = new_request["Content-Type:"].substr(v, 8);
        std::string value = new_request["Content-Type:"].substr(new_request["Content-Type:"].find("=") + 1);
        new_request[key] = value;
        new_request["Content-Type:"] = new_request["Content-Type:"].substr(0, new_request["Content-Type:"].find(";"));
    }
    return new_request;
}

void HTTP_server::ProcessUpload(std::vector<Request>::iterator req)
{
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    int n = recv(req->client_fd, buf, BUF_SIZE, MSG_DONTWAIT | MSG_PEEK);
    if (n < 0)
    {
        perror("Error receiving data from client1");
        exit(EXIT_FAILURE);
    }
    char *header = strstr(buf, "\r\n\r\n");
    size_t headerlength = header - buf + 4;

    // Get lines of Header
    memset(buf, 0, BUF_SIZE);
    n = recv(req->client_fd, buf, headerlength, MSG_DONTWAIT);
    if (n < 0)
    {
        perror("Error receiving data from client2");
        exit(EXIT_FAILURE);
    }
    
    std::string fileheader(buf);
    // std::cout << buf << std::endl;
    std::string filename = fileheader.substr(fileheader.find("filename=") + 10);
    filename = filename.substr(0, filename.find("\""));
    req->path += filename;
    std::ofstream createFile(req->path, std::ios::binary | std::ios::trunc);

    if (!createFile.is_open())
    {
        perror("error creating uploading file");
        exit(1);
    }

    size_t bodyLength = std::atol(req->requestHeaderMap["Content-Length:"].c_str()) - headerlength;
    size_t boundaryLength = req->requestHeaderMap["boundary"].size() + 4;
    size_t readbytes = 0;
    size_t remainingBytes = bodyLength - boundaryLength;

    while (remainingBytes > boundaryLength)
    {
        size_t chunkBytes = std::min((size_t) BUF_SIZE, remainingBytes);

        memset(buf, 0, chunkBytes);
        readbytes = recv(req->client_fd, buf, chunkBytes, MSG_DONTWAIT);
        if (readbytes < 0)
        {
            perror("Error receiving data from client in Upload");
            exit(EXIT_FAILURE);
        }

        createFile.write(buf, readbytes);
        remainingBytes -= readbytes;
    }
    createFile.close();
    memset(buf, 0, BUF_SIZE);
    req->GenerateUploadResponse();
    recv(req->client_fd, buf, BUF_SIZE, MSG_DONTWAIT);
}

void HTTP_server::get_request(int i, std::vector<Request>::iterator req)
{
    if (!req->initialResponseSent)
    {

        if (req->responsebody.empty())
        {
            int file_fd = open(req->path.c_str(), O_RDONLY);
            if (file_fd < 0)
            {
                perror("Error opening file");
                throw(InvalidFileDownloadException());
            }
            req->file_fd = file_fd;
        }

        if (send(req->client_fd, req->ResponseHeader.c_str(), req->ResponseHeader.size(), 0) < 0)
        {
            perror("Error sending initial response headers");
            exit(EXIT_FAILURE);
        }

        if (!req->responsebody.empty())
        {
            if (send(req->client_fd, req->responsebody.c_str(), req->responsebody.size(), 0) == -1)
                perror("Autoindex send error");
            req->requestdone = true;
            return;
        }
        req->initialResponseSent = true;
    }
    const int chunkSize = BUF_SIZE;
    char buffer[chunkSize];
    ssize_t bytesRead;
    bytesRead = read(req->file_fd, buffer, chunkSize);
    if (bytesRead > 0)
    {
        std::stringstream chunkSizeHex;
        chunkSizeHex << std::hex << bytesRead << "\r\n";
        std::string chunkSizeHexStr = chunkSizeHex.str();
        std::string chunkData(buffer, bytesRead);
        std::string chunk = chunkSizeHexStr + chunkData + "\r\n";
        ssize_t bytesSent = send(req->client_fd, chunk.c_str(), chunk.length(), 0);
        if (bytesSent < 0)
        {
            perror("Error sending chunk");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::string lastChunk = "0\r\n\r\n";
        if (send(req->client_fd, lastChunk.c_str(), lastChunk.length(), 0) < 0)
        {
            perror("Error sending last chunk");
            exit(EXIT_FAILURE);
        }
        req->requestdone = true;
    }
    FdsClients[i].second.lastInteractionTime = std::time(nullptr);
}

std::string HTTP_server::toHex(int value)
{
    std::stringstream stream;
    stream << std::hex << value;
    return stream.str();
}

bool HTTP_server::CheckForTimeout(int i)
{
    currentTime = std::time(nullptr);

    time_t elapsedDuration = currentTime - FdsClients[i].second.lastInteractionTime;
    if (elapsedDuration >= timeoutDuration)
        return true;
    return false;
}

void HTTP_server::server_loop()
{
    while (true)
    {
        server_conducts_poll();
        for (int i = 0; i < listening_port_no; i++)
            server_port_listening(i);
        for (std::set<int>::iterator it_idx = activeClientIdx.begin(); it_idx != activeClientIdx.end(); it_idx++)
        {
            if (fds[*it_idx].revents & POLLIN)
            {
                Request new_req;
                if (FdsClients[*it_idx].second.server_full)
                    new_req.GenerateServerError(503, ConfigVec[FdsClients[*it_idx].second.socket]);
                else
                {
                    new_req.requestHeaderMap = server_mapping_request(*it_idx);
                    new_req.CreateResponse(ConfigVec[FdsClients[*it_idx].second.socket]);
                    FdsClients[*it_idx].second.lastInteractionTime = std::time(nullptr);
                }
                new_req.client_fd = FdsClients[*it_idx].first;
                FdsClients[*it_idx].second.RequestVector.push_back(new_req);
                if (new_req.cutoffClient)
                    fds[*it_idx].events = POLLOUT;
                else if (new_req.isUpload)
                    ProcessUpload(FdsClients[*it_idx].second.RequestVector.end() - 1);
                else if (new_req.isCGI)
                {
                    Cgi cgi("generic cgi", new_req.id);
                    try{
                        cgi.run(new_req.requestHeaderMap);
                    }
                    catch (const std::exception &e){
                        std::cerr << e.what();
                    }
                    (void)fds;
                }
            }
            if (fds[*it_idx].revents & POLLOUT)
            {
                for (std::vector<Request>::iterator it_req = FdsClients[*it_idx].second.RequestVector.begin(); it_req != FdsClients[*it_idx].second.RequestVector.end(); it_req++)
                {
                    try
                    {
                        get_request(*it_idx, it_req);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what();
                    }
                    if (it_req->requestdone)
                    {
                        if (it_req->cutoffClient)
                            FdsClients[*it_idx].second.cutoffClient = true;
                        std::cout << "Response: " << it_req->id << " sent to client: " << *it_idx << std::endl;
                        FdsClients[*it_idx].second.RequestVector.erase(it_req);
                        break;
                    }
                }
            }
            if (fds[*it_idx].revents & (POLLHUP | POLLERR) || FdsClients[*it_idx].second.cutoffClient || (FdsClients[*it_idx].second.RequestVector.empty() && (CheckForTimeout(*it_idx))))
            {
                close(fds[*it_idx].fd);
                fds[*it_idx].events = POLLIN | POLLOUT;
                fds[*it_idx].fd = -1;
                FdsClients[*it_idx].second.ResetClient();
                std::cout << "Connection Timeout - Client " << *it_idx << " disconnected" << std::endl;
                activeClientIdx.erase(it_idx);
                break;
            }
        }
    }
    for (std::set<int>::iterator it_idx = activeClientIdx.begin(); it_idx != activeClientIdx.end(); it_idx++)
    {
        close(FdsClients[*it_idx].first);
        close(fds[*it_idx].fd);
    }
}

void HTTP_server::InitFdsClients()
{
    for (int i = 0; i < (MAX_CLIENTS + listening_port_no + 1); i++)
    {
        Client init;
        std::pair<int, Client> tmp(-1, init);
        FdsClients.push_back(tmp);
    }
}

int HTTP_server::running()
{
    ConfigCheck check;

    listening_port_no = check.checkConfig(_path);
    fds = new struct pollfd[MAX_CLIENTS + listening_port_no + 1];
    InitFdsClients();

    for (int i = 0; i < listening_port_no; i++)
    {
        ServerConfig tmp(_path, i);
        Socket socket(atoi(tmp.port.c_str()), tmp.host);
        FdsClients[i].first = socket.server_fd;
        ConfigVec.push_back(tmp);
        std::cout << "Socket " << (i + 1) << " (FD " << FdsClients[i].first
                  << ") is listening on: " << tmp.getConfProps("listen:") << std::endl;
    }
    create_pollfd_struct();
    server_loop();
    return 0;
}
