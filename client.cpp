
#include <arpa/inet.h>  // for htonl() htons() ntohl() ntohs()
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
using namespace std;
using namespace filesystem;

int main(int argc, char *argv[]) {
    create_directories("./client_dir");
    if (argc != 2) {
        printf("usage: %s [ip:port]\n", argv[0]);
        exit(1);
    }
    // read arguments
    int t1, t2, t3, t4, port;
    sscanf(argv[1], "%d.%d.%d.%d:%d", &t1, &t2, &t3, &t4, &port);
    char ip[20] = {};
    sprintf(ip, "%d.%d.%d.%d", t1, t2, t3, t4);

    // init client
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) ERR_EXIT("socket");
    
    struct sockaddr_in cliaddr;
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr(ip);
    cliaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1) ERR_EXIT("connect");

    fprintf(stderr, "\nstarting on %s:%d, fd = %d\n", ip, port, sockfd);
    

    char buf[100] = {}, name[100] = {};
    recv(sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf);

    scanf("%s", name);
    write(sockfd, name, sizeof(name));
    recv(sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf);
    while(strcmp(buf, "username is in used, please try another:") == 0){
        scanf("%s", name);
        write(sockfd, name, sizeof(name));
        recv(sockfd, buf, sizeof(buf), 0);
        puts(buf);
    }
    // instruction
    string s;
    getline(cin, s); // TODO: why?????
    while (1){
        getline(cin, s);
        cout << s << endl;
        istringstream in(s);
        vector<string> v;
        string t;
        while(in >> t) v.push_back(t);
            
        if (v[0] != "ls" and v[0] != "get" and v[0] != "put"){
            printf("Command not found\n");
            continue;
        }
        if((v[0] == "ls" and v.size() != 1) or (v[0] == "get" and v.size() != 2) or (v[0] == "put" and v.size() != 2)){
            printf("Command format error\n");
            continue;
        }

        if(v[0] == "get"){
            write(sockfd, s.c_str(), 100);
            recv(sockfd, buf, sizeof(buf), MSG_WAITALL);
            cerr << buf << endl; // debug
            // read file content from server
            string clientpath = "./client_dir/" + v[1];
            char filebuf[2048] = {};
            FILE *wr_fp = fopen(clientpath.c_str(), "wb");
            int filesz;
            recv(sockfd, &filesz, sizeof(filesz), MSG_WAITALL);
            cout << "got size " << filesz << endl;
            while(filesz > 0){
                int suc = recv(sockfd, filebuf, sizeof(filebuf), MSG_WAITALL);
                cout << "remaining: " << filesz << endl;
                fwrite(filebuf, sizeof(char), min((int)sizeof(filebuf), filesz), wr_fp);
                fflush(wr_fp);
                filesz -= suc;
            }
            recv(sockfd, buf, sizeof(buf), MSG_WAITALL);
            cout << buf << endl;
        }
        else if(v[0] == "put"){
            // file to put does not exist
            string clientpath = "./client_dir/" + v[1];
            if(not exists(clientpath)){
                printf("The %s doesn't exist\n", v[1].c_str());
                continue;
            }
            write(sockfd, s.c_str(), 100); // write ins, filename to server
            int filesz = file_size(clientpath);
            write(sockfd, &filesz, sizeof(filesz));
            // put the file to server
            char filebuf[2048] = {};
            FILE *put_fp = fopen(clientpath.c_str(), "rb");
            while(fread(filebuf, sizeof(char), 2048, put_fp) > 0){
                int tot = sizeof(filebuf);
                int remain = sizeof(filebuf);
                int suc = write(sockfd, filebuf, sizeof(filebuf)); // write file content
                remain -= suc;
                while(remain > 0){
                    remain -= write(sockfd, filebuf+(tot-remain), remain);
                }
            }
            printf("put %s successfully\n", v[1].c_str());
        }
        else if(v[0] == "ls"){
            write(sockfd, s.c_str(), 100);
            char lsbuf[2048] = {};
            recv(sockfd, lsbuf, sizeof(lsbuf), MSG_WAITALL);
            cout << lsbuf << endl;
        }
    }
    close(sockfd);
    return 0;
}