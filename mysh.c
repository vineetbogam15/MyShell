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

// Directory paths to search for executables
#define DIR_PATHS {"/usr/local/bin", "/usr/bin", "/bin"}

// Function prototypes
void print_prompt();
void read_command(char* command);
void parse_command(char* command, char* tokens[]);
void execute_command(char* tokens[]);
void execute_builtin_command(char* tokens[]);
void print_welcome_message();
void print_goodbye_message();
int check_slash(char * command);

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
        tokens[token_count] = malloc(strlen(token));
        strcpy(tokens[token_count], token);
        token_count++;
        token = strtok(NULL, " \t\n");
    }
    tokens[token_count] = '\0';
}

int check_slash(char * command) {
    int i = 0;
    int state = 0;
    while (command[i] != '\0') {
        if (command[i] == '/') {
            state = 1;
            break;
        }
        i++;
    }
    return state;
}

void execute_command(char* tokens[]) {
    // Check if the command is a built-in command
    if (strcmp(tokens[0], "cd") == 0 || strcmp(tokens[0], "pwd") == 0 || 
        strcmp(tokens[0], "which") == 0 || strcmp(tokens[0], "exit") == 0) {
        execute_builtin_command(tokens);
    } else if (check_slash(tokens[0]) == 0) {
        // Check if the command is in the specified directories
        char* dir_paths[] = DIR_PATHS;
        int i = 0;
        int found = 0;
        while (dir_paths[i] != NULL) {
            char path[MAX_COMMAND_LENGTH];
            //copy full path of desired command into path
            strcpy(path, dir_paths[i]);
            strcat(path, "/");
            strcat(path, tokens[0]);
            if (access(path, X_OK) == 0) {
                // Found the command in one of the directories
                found = 1;
                // Execute the command
                //Create a child process
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    execv(path, tokens);
                    // error if execv returns
                    perror("execv");
                    exit(EXIT_FAILURE);
                } else if (pid < 0) {
                    // Fork failed
                    perror("fork");
                } else {
                    // Parent process
                    int status;
                    waitpid(pid, &status, 0);
                }
                break;
            }
            i++;
        }

        if (!found) {
            // Command not found in specified directories
            printf("Command not found: %s\n", tokens[0]);
        }
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execv(tokens[0], tokens);
            // error if execv returns
            perror("execv");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Fork failed
            perror("fork");
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
        }

    }
}

void free_tokens(char* tokens[]) {
    int i = 0;
    while (i < MAX_TOKENS-1 && tokens[i] != NULL) {
        free(tokens[i]);
        i++;
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
    } else if (strcmp(tokens[0], "exit") == 0) {
        int j = 1;
        while (j < MAX_TOKENS-1 && tokens[j] != NULL) {
            printf("%s ", tokens[j]);
            j++;
        }
        printf("\nExitting mysh\n");
        free_tokens(tokens);
        exit(EXIT_SUCCESS);
    } 
   
}
