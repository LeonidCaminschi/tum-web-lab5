#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <boost/regex.hpp>

std::string makeHttpRequest(std::string &url)
{
    std::string originalUrl = url;
    int maxRedirects = 5; // Maximum number of redirects to follow

    while (maxRedirects > 0)
    {
        // Parse the URL to extract host and path
        std::regex urlRegex("(https?)://([^/]+)(/.*)?");
        std::smatch match;
        if (!std::regex_match(url, match, urlRegex))
        {
            std::cerr << "Invalid URL format" << std::endl;
            return "";
        }
        std::string protocol = match[1];
        std::string host = match[2];
        std::string path = match[3].length() > 0 ? match[3].str() : "/";

        // Open socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            std::cerr << "Failed to create socket" << std::endl;
            return "";
        }

        // Resolve hostname
        struct sockaddr_in server;
        struct hostent *hostinfo;
        hostinfo = gethostbyname(host.c_str());
        if (!hostinfo)
        {
            std::cerr << "Failed to resolve hostname" << std::endl;
            close(sock);
            return "";
        }

        // Fill in server structure
        server.sin_family = AF_INET;
        server.sin_port = htons(443);
        memcpy(&server.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

        // Connect to server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            std::cerr << "Connection failed" << std::endl;
            close(sock);
            return "";
        }

        // Initialize OpenSSL
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();

        // Create a new SSL context
        SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());

        if (ctx == nullptr)
        {
            std::cerr << "Error creating SSL context" << std::endl;
            return "";
        }

        // Create a new SSL connection
        SSL *ssl = SSL_new(ctx);

        if (ssl == nullptr)
        {
            std::cerr << "Error creating SSL connection" << std::endl;
            return "";
        }

        // Connect the SSL object with a file descriptor
        if (!SSL_set_fd(ssl, sock))
        {
            std::cerr << "Error connecting the SSL object with a file descriptor" << std::endl;
            return "";
        }

        // Perform the SSL/TLS handshake
        if (SSL_connect(ssl) != 1)
        {
            std::cerr << "Error performing the SSL/TLS handshake" << std::endl;
            ERR_print_errors_fp(stderr);
            return "";
        }

        // Send HTTP request
        std::string httpRequest = "GET " + path + " HTTP/1.1\r\n";
        httpRequest += "Host: " + host + "\r\n";
        httpRequest += "Connection: keep-alive\r\n";
        httpRequest += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n";
        // httpRequest += "Accept-Encoding: gzip\r\n";
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
        SSL_write(ssl, httpRequest.c_str(), httpRequest.length());

        std::string responseHeaders;
        char buffer[8192];
        int bytes;

        while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) // Subtract 1 to leave space for null terminator
        {
            buffer[bytes] = 0; // Null-terminate the buffer
            responseHeaders += buffer;

            // Stop reading once we've read the headers
            // if (responseHeaders.find("\r\n\r\n") != std::string::npos)
            // {
            //     break;
            // }
        }

        //std::cout << responseHeaders << std::endl;

        // Don't forget to free the resources
        SSL_free(ssl);
        SSL_CTX_free(ctx);

        // Check for redirect
        // Check for redirect
        std::string responseStr(responseHeaders.begin(), responseHeaders.end());
        // std::cout << responseStr << std::endl;
        if (responseStr.find("301 Moved Permanently") != std::string::npos ||
            responseStr.find("302 Found") != std::string::npos)
        {
            // Extract new location from response header
            std::regex locationRegex("location: (.+)\r\n", std::regex_constants::icase);
            std::smatch locationMatch;
            if (std::regex_search(responseStr, locationMatch, locationRegex))
            {
                url = locationMatch[1];
                maxRedirects--;
                continue; // Try again with new URL
            }
            else
            {
                std::cerr << "Error parsing redirect location" << std::endl;
                close(sock);
                return "";
            }
        }

        // Close socket
        close(sock);
        return responseHeaders;
        break; // Exit loop if not redirected
    }
    return "";
}

void parseSearch(const std::string &response)
{
    // Find the title
    std::regex titleRegex("<title>(.*?)</title>");
    std::smatch titleMatch;
    if (std::regex_search(response, titleMatch, titleRegex))
    {
        std::string title = titleMatch[1];
        std::cout << "Title: " << title << std::endl;
    }

    // Find the description
    std::regex descriptionRegex("<meta name=\"description\" content=\"(.*?)\">");
    std::smatch descriptionMatch;
    if (std::regex_search(response, descriptionMatch, descriptionRegex))
    {
        std::string description = descriptionMatch[1];
        std::cout << "Description: " << description << std::endl;
    }

    // Find the link
    std::regex linkRegex("<a href=\"(.*?)\"");
    std::smatch linkMatch;
    if (std::regex_search(response, linkMatch, linkRegex))
    {
        std::string link = linkMatch[1];
        std::cout << "Link: " << link << std::endl;
    }
}

void parseLink(std::string response)
{
    // Remove response headers
    size_t endOfHeaders = response.find("\r\n\r\n");
    if (endOfHeaders != std::string::npos)
    {
        response = response.substr(endOfHeaders + 4); // Skip over "\r\n\r\n"
    }

    // Remove HTML tags and CSS
    boost::regex htmlTagRegex("<[^>]*>");
    boost::regex cssRegex("<style.*?>.*?</style>", boost::regex::icase | boost::regex::extended);

    response = boost::regex_replace(response, cssRegex, "");
    response = boost::regex_replace(response, htmlTagRegex, "");

    // Now response contains the text content of the HTTP response, with HTML tags and CSS removed
}

// Function to search using a search engine and print the top results
void search(const std::string &searchTerm)
{
    std::string searchUrl = "http://www.google.com/search?q=" + searchTerm;
    std::string response = makeHttpRequest(searchUrl);
    parseSearch(response);
}

void link(std::string &url)
{
    std::string response = makeHttpRequest(url);
    parseLink(response);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: go2web -u <URL> | -s <search-term> | -h" << std::endl;
        return 1;
    }

    std::string option = argv[1];
    if (option == "-h")
    {
        std::cout << "Usage: go2web -u <URL> | -s <search-term> | -h" << std::endl;
    }
    else if (option == "-u" && argc == 3)
    {
        std::string url = argv[2];
        link(url);
    }
    else if (option == "-s" && argc == 3)
    {
        std::string searchTerm = argv[2];
        search(searchTerm);
    }
    else
    {
        std::cerr << "Invalid arguments. Use -h for help." << std::endl;
        return 1;
    }

    return 0;
}
