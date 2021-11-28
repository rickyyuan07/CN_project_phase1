#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <iostream>
#include <filesystem>
#include <string>
#include <set>
#include <vector>
#include <algorithm>

using namespace std;
using namespace filesystem;

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

set<string> user_id_set;
void* serve(void* _fd){
    int fd = (int64_t)_fd;
    write(fd, "input your username:", sizeof("input your username:"));
    char name[100] = {};
    recv(fd, name, sizeof(name), MSG_WAITALL);
    while (user_id_set.count(name) == 1){
        write(fd, "username is in used, please try another:", sizeof("username is in used, please try another:"));
        recv(fd, name, sizeof(name), MSG_WAITALL);
    }
    write(fd, "connect successfully", sizeof("connect successfully"));
    string tmp(name);
    user_id_set.insert(tmp);

    while (1) {
        char ins[100] = {};
        // check if socket is still alive
        int tmp = recv(fd, ins, sizeof(ins), MSG_PEEK);
        if(tmp == 0){
            fprintf(stderr, "client fd: %d has closed.\n", fd);
            user_id_set.erase(name);
            break;
        }
        
        recv(fd, ins, sizeof(ins), MSG_WAITALL);
        fprintf(stderr, "got message from client %d typed: \"%s\" size 100=%d\n", fd, ins, tmp); // DEBUG

        string s(ins);
        istringstream in(s);
        vector<string> v;
        string t;
        while(in >> t) v.push_back(t);
        
        if(v[0] == "get"){
            string serverpath = "./server_dir/" + v[1];
            // file to put does not exist
            if(not exists(serverpath)){
                sprintf(ins, "The %s doesn't exist\n", v[1].c_str());
                write(fd, ins, sizeof(ins));
                fprintf(stderr, "To client %d: file: \"%s\" not exists\n", fd, v[1].c_str()); // DEBUG
                continue;
            }
            else{
                sprintf(ins, "file exists, start downloading\n");
                write(fd, ins, sizeof(ins));
                fprintf(stderr, "To client %d: server start uploading \"%s\"\n", fd, v[1].c_str()); // DEBUG
            }
            
            int filesz = file_size(serverpath);
            write(fd, &filesz, sizeof(filesz));
            // get the file from server
            char filebuf[2048] = {};
            FILE *put_fp = fopen(serverpath.c_str(), "rb");
            while(fread(filebuf, sizeof(char), 2048, put_fp) > 0){
                if(send(fd, filebuf, sizeof(filebuf), MSG_NOSIGNAL) < 0){ // write file content
                    fprintf(stderr, "client %d has closed, file transmission stops\n", fd);
                    fclose(put_fp);
                    user_id_set.erase(name);
                    return 0;
                }
            }
            fclose(put_fp);
            sprintf(ins, "get %s successfully\n", v[1].c_str());
            send(fd, ins, sizeof(ins), MSG_NOSIGNAL);
            fprintf(stderr, "To client %d: file: \"%s\" get successfully\n", fd, v[1].c_str()); // DEBUG
        }
        else if(v[0] == "put"){
            string serverpath = "./server_dir/" + v[1];
            // read file content from client
            char filebuf[2048] = {};
            FILE *wr_fp = fopen(serverpath.c_str(), "wb");
            int filesz;
            recv(fd, &filesz, sizeof(filesz), MSG_WAITALL);
            fprintf(stderr, "put from client %d: file name \"%s\" with size = %d\n", fd, v[1].c_str(), filesz); // DEBUG
            while(filesz > 0){
                int suc = recv(fd, filebuf, sizeof(filebuf), MSG_WAITALL);
                if(suc != sizeof(filebuf)){
                    cerr << "did not receive correct number of bytes: " << sizeof(filebuf) << endl;
                }
                fwrite(filebuf, sizeof(char), min((int)sizeof(filebuf), filesz), wr_fp);
                filesz -= suc;
            }
            fprintf(stderr, "Server successfully put from client %d\
            : file name \"%s\" with size = %d\n", fd, v[1].c_str(), filesz); // DEBUG
        }
        else if(v[0] == "ls"){
            vector<string> ls_vector;
            for (const auto & entry : directory_iterator("./server_dir/"))
                ls_vector.push_back((string)entry.path());
            
            sort(ls_vector.begin(), ls_vector.end());
            string to_client;
            for(string s : ls_vector){
                to_client += s.substr(13) + "\n"; // 13 for sizeof("./server_dir/")
            }
            write(fd, to_client.c_str(), 2048);
        }
    }
    close(fd);
    user_id_set.erase(name);
    return 0;
}
const int client_num = 10;
int main(int argc, char* argv[]) {
    create_directories("./server_dir");
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }
    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;
    
    //init server
    int svr_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_fd < 0) ERR_EXIT("socket");
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    if (bind(svr_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
    fprintf(stderr, "\nstarting on port:%d, fd %d\n", atoi(argv[1]), svr_fd);

    
    // char hostbuffer[256];
    // struct hostent *host_entry;
    // host_entry = gethostbyname(hostbuffer);
    // cout << inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])) << endl;

    // Loop for handling connections
    // fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr_fd, maxfd);
    clilen = sizeof(cliaddr);
    pthread_t t[client_num];
    while (1) {
        int conn_fd = accept(svr_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
        fprintf(stderr, "getting a new request... fd %d\n", conn_fd);
        pthread_create(t, NULL, serve, (void *)(int64_t)conn_fd);
    }
    return 0;
}
