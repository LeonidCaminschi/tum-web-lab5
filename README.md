# General overview
Basic application of accesing websockets in order to achieve the following results with the following commands

```
go2web -u <URL>         # make an HTTP request to the specified URL and print the response
go2web -s <search-term> # make an HTTP request to search the term using your favorite search engine and print top 10 results
go2web -h               # show this help
```

# Building process

Use the following commands in order to get a CLI executable
```
g++ main.cpp -o go2web -lssl -lcrypto -lz -lboost_regex
```
After which a new binary file will be generated **go2web**

# Features
- CLI Application
- Ability to search by term in CLI with results from Google Search
- Ability to request a resource from a given link/url
- Ability to rerequest data in case API response 300 is recieved

