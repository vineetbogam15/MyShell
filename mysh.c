#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#define MAX_COMMAND_LENGTH 1000
#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 100

// Function prototypes
void print_prompt();
void read_command(char* command);
void parse_command(char* command, char* tokens[]);
void execute_command(char* tokens[]);
void execute_builtin_command(char* tokens[]);
void print_welcome_message();
void print_goodbye_message();

int main(int argc, char* argv[]) {
    // Determine mode of operation (interactive or batch)
    bool interactive_mode = isatty(STDIN_FILENO);

    // If interactive mode, print welcome message
    if (interactive_mode) {
        print_welcome_message();
    }
   
    // Main loop to read and execute commands
    while (1) {
        char command[MAX_COMMAND_LENGTH];
        char* tokens[MAX_TOKENS];

         if (interactive_mode) {
            print_prompt();
        }

        // Read command from input
        read_command(command);

        // Parse command into tokens
        parse_command(command, tokens);

        // Execute the command
        execute_command(tokens);
    }

    // If interactive mode, print goodbye message
    if (interactive_mode) {
        print_goodbye_message();
    }

    return 0;
}

void print_prompt() {
    printf("mysh> ");
}

void print_welcome_message() {
    printf("Welcome to my shell!\n");
}

void print_goodbye_message() {
    printf("exiting\n");
}

void read_command(char* command) {
     fgets(command, MAX_COMMAND_LENGTH, stdin);
}

void parse_command(char* command, char* tokens[]) {
    char* token;
    int token_count = 0;

    // Split the command into tokens
    token = strtok(command, " \t\n");
    while (token != NULL && token_count < MAX_TOKENS - 1) {
        tokens[token_count] = strdup(token);
        token_count++;
        token = strtok(NULL, " \t\n");
    }
    tokens[token_count] = '\0';
}

void execute_command(char* tokens[]) {
    // Check if the command is a built-in command
    if (strcmp(tokens[0], "cd") == 0 || strcmp(tokens[0], "pwd") == 0 || 
        strcmp(tokens[0], "which") == 0 || strcmp(tokens[0], "exit") == 0) {
        execute_builtin_command(tokens);
    } else {
        //Creates a subprocess for the bare name command 
    }
}

void execute_builtin_command(char* tokens[]) {
    if (strcmp(tokens[0], "cd") == 0) {
        // Change directory
        if (tokens[1] != NULL) {
            if (chdir(tokens[1]) != 0) {
                perror("cd");
            }
        } else {
            fprintf(stderr, "cd: missing argument\n");
        }
    } else if (strcmp(tokens[0], "pwd") == 0) {
        // Print working directory
        char cwd[1000];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
    } 
   
}

