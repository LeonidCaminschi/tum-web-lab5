#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

std::string makeHttpRequest(std::string& url) {
    std::string originalUrl = url;
    int maxRedirects = 5; // Maximum number of redirects to follow

    while (maxRedirects > 0) {
        // Parse the URL to extract host and path
        std::regex urlRegex("(https?)://([^/]+)(/.*)?");
        std::smatch match;
        if (!std::regex_match(url, match, urlRegex)) {
            std::cerr << "Invalid URL format" << std::endl;
            return "";
        }
        std::string protocol = match[1];
        std::string host = match[2];
        std::string path = match[3].length() > 0 ? match[3].str() : "/";

    // Open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return "";
    }

    // Resolve hostname
    struct sockaddr_in server;
    struct hostent* hostinfo;
    hostinfo = gethostbyname(host.c_str());
    if (!hostinfo) {
        std::cerr << "Failed to resolve hostname" << std::endl;
        close(sock);
        return "";
    }

    // Fill in server structure
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    memcpy(&server.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(sock);
        return "";
    }

        // Send HTTP request
        std::string httpRequest = "GET " + path + " HTTP/1.1\r\n";
        httpRequest += "Host: " + host + "\r\n";
        httpRequest += "Connection: close\r\n";
        httpRequest += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n";
        httpRequest += "Accept-Encoding: gzip, deflate, br, zstd\r\n";
        httpRequest += "Accept-Language: en-US,en;q=0.9\r\n";
        httpRequest += "Sec-Ch-Ua: \"Brave\";v=\"123\", \"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"123\"\r\n";
        httpRequest += "Sec-Ch-Ua-Mobile: ?0\r\n";
        httpRequest += "Sec-Ch-Ua-Platform: \"Windows\"\r\n";
        httpRequest += "Sec-Fetch-Dest: document\r\n";
        httpRequest += "Sec-Fetch-Mode: navigate\r\n";
        httpRequest += "Sec-Fetch-Site: none\r\n";
        httpRequest += "Sec-Fetch-User: ?1\r\n";
        httpRequest += "Sec-Gpc: 1\r\n";
        httpRequest += "Upgrade-Insecure-Requests: 1\r\n";
        httpRequest += "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36\r\n";
        httpRequest += "\r\n";
        send(sock, httpRequest.c_str(), httpRequest.length(), 118);

        // Receive and print HTTP response
        std::vector<char> response(4096);
        int bytesReceived = recv(sock, response.data(), response.size(), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Error receiving response" << std::endl;
            close(sock);
            return "";
        }

        // std::cout << std::string(response.begin(), response.begin() + bytesReceived) << std::endl;
        std::cout << httpRequest << std::endl;
        std::cout << response.data() << std::endl;
        exit(0);

        // Check for redirect
        std::string responseStr(response.begin(), response.begin() + bytesReceived);
        std::cout << responseStr << std::endl;
        if (responseStr.find("301 Moved Permanently") != std::string::npos ||
            responseStr.find("302 Found") != std::string::npos) {
            // Extract new location from response header
            std::regex locationRegex("location: (.+)\r\n");
            std::smatch locationMatch;
            if (std::regex_search(responseStr, locationMatch, locationRegex)) {
                url = locationMatch[1];
                maxRedirects--;
                continue; // Try again with new URL
            } else {
                std::cerr << "Error parsing redirect location" << std::endl;
                close(sock);
                return "";
            }
        }

        // Print response
        std::cout.write(response.data(), bytesReceived);

        // Close socket
        close(sock);
        return std::string(response.begin(), response.begin() + bytesReceived);
        break; // Exit loop if not redirected
    }
    return "";
}


void parseResponse(const std::string& response) {
    // Find the title
    std::regex titleRegex("<title>(.*?)</title>");
    std::smatch titleMatch;
    if (std::regex_search(response, titleMatch, titleRegex)) {
        std::string title = titleMatch[1];
        std::cout << "Title: " << title << std::endl;
    }

    // Find the description
    std::regex descriptionRegex("<meta name=\"description\" content=\"(.*?)\">");
    std::smatch descriptionMatch;
    if (std::regex_search(response, descriptionMatch, descriptionRegex)) {
        std::string description = descriptionMatch[1];
        std::cout << "Description: " << description << std::endl;
    }

    // Find the link
    std::regex linkRegex("<a href=\"(.*?)\"");
    std::smatch linkMatch;
    if (std::regex_search(response, linkMatch, linkRegex)) {
        std::string link = linkMatch[1];
        std::cout << "Link: " << link << std::endl;
    }
}


// Function to search using a search engine and print the top results
void search(const std::string& searchTerm) {
    std::string searchUrl = "http://www.google.com/search?q=" + searchTerm;
    std::string response = makeHttpRequest(searchUrl);
    parseResponse(response);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: go2web -u <URL> | -s <search-term> | -h" << std::endl;
        return 1;
    }

    std::string option = argv[1];
    if (option == "-h") {
        std::cout << "Usage: go2web -u <URL> | -s <search-term> | -h" << std::endl;
    } else if (option == "-u" && argc == 3) {
        std::string url = argv[2];
        makeHttpRequest(url);
    } else if (option == "-s" && argc == 3) {
        std::string searchTerm = argv[2];
        search(searchTerm);
    } else {
        std::cerr << "Invalid arguments. Use -h for help." << std::endl;
        return 1;
    }

    return 0;
}
