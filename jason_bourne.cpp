//======================================================================//
//= jason_bourne.c                                                     =//
//= bourne, into BourneAgain SHell (BASH),                             =//
//= I offer you:                                                       =//
//= Jason Bourne                                                       =//
//======================================================================//
//= Definitions                                                        =//
//======================================================================//

#define SHELL_PROMPT "JasonBourne"
#define CMD_BUFFER 4096
#define MAX_ARGS 32

//======================================================================//
//= Includes                                                           =//
//======================================================================//

#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


//======================================================================//
//= Main                                                               =//
//======================================================================//


int main() 
{
    std::string raw_input;
    char raw_buffered[CMD_BUFFER];
    char tokenized[CMD_BUFFER];
    char *argv[32];
    int optPipe[2];
    int  argc = 0, cmd_i = -1;
    int  pipe_i = -1, redir_i = -1;
    int  file_descriptor;
    char *special_token = NULL;

    // std::cout << std::endl;
    // Main Loop
    while (true)
    {
        argc = 0; cmd_i = -1;
        redir_i = -1; pipe_i = -1;
        std::cout << SHELL_PROMPT << ':' <<  "> ";
        std::getline(std::cin, raw_input);
        if (raw_input.size() > CMD_BUFFER)
        {
            std::cout << "[ERROR] Command too long! Try Again :(\n";
            continue;
        }
        strcpy(raw_buffered, raw_input.c_str());
        char *token = strtok(raw_buffered, " ");

        // Need safe close state
        if (!strcmp(token, "exit"))
            break;


        // Tokenize String
        while (token != NULL)
        {
            argv[argc] = token;
            if (
                !strcmp(token, "|") || 
                !strcmp(token, ">"))
            {
                // argc++;
                // argv[argc] = NULL;
                cmd_i = argc;
            }
            argc++;
            token = strtok(NULL," ");
        }
        argv[argc] = NULL;
        // printf("%s\n", argv[0]);
        // std::cout << argc << std::endl;
        // std::cout << cmd_i << std::endl;
        if (cmd_i != -1)
            special_token = argv[cmd_i];
        for (int i = 0; i < argc; i++) 
        {
            if (!strcmp(argv[i], "|")) 
                pipe_i = i;
            else if (!strcmp(argv[i], ">")) 
                redir_i = i;
        }
        char **left;
        char **right;
        if (pipe_i != -1) 
        {
            argv[pipe_i] = NULL;
            pipe(optPipe);
        }
        char *outfile = NULL;
        int outFileDescriptor;
        if (redir_i != -1) 
        {
            argv[redir_i] = NULL;
            if (redir_i + 1 >= argc)
            {
                std::cerr << "Missing output file\n"; 
                continue; 
            }
            outfile = argv[redir_i + 1];
            outFileDescriptor = open(outfile,O_CREAT|O_RDWR|O_TRUNC,0644);
        }
        if (cmd_i != -1)
        {
            left = argv;
            right = &argv[pipe_i + 1];
        }
        
        if (fork() == 0) // Child process
        {
            if (pipe_i == -1 && redir_i == -1) // Regular
                execvp(argv[0], argv);
            else if (pipe_i == -1 && redir_i != -1) 
            {
                // No pipe, file redirect
                dup2(outFileDescriptor, 1);
                execvp(left[0], left);
                std::cerr << "Invalid Command " << left[0] << '\n'; 
                exit(127);
            }
            else 
            {
                // Pipe, no redirect
                // Pipe and redirect
                // Both run left hand regardless
                dup2(optPipe[1], 1); 
                close(optPipe[0]);
                execvp(left[0], left);
                std::cerr << "Invalid Command " << left[0] << '\n'; 
                exit(127); 
            }
        }
        else if (pipe_i != -1 && fork() == 0) // Other Child if pipe
        {
            if(redir_i == -1)
            {
                dup2(optPipe[0], 0); 
                close(optPipe[1]);
                execvp(right[0], right);
                std::cerr << "Invalid Command " << right[0] << '\n'; 
                exit(127); 
            }
            else 
            {
                dup2(optPipe[0], 0); 
                close(optPipe[1]);
                dup2(outFileDescriptor, 1);
                execvp(right[0], right);
                std::cerr << "Invalid Command " << right[0] << '\n'; 
                exit(127); 
            }
        }
        else // Parent
        {
            if (pipe_i != -1) 
            {
                close(optPipe[0]);
                close(optPipe[1]);
            }
            wait(0);wait(0);

            if (redir_i != -1)
            {
                close(outFileDescriptor);
            }
        }
    }    
    return 0;
}
