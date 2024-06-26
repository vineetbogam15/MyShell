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
#include <sys/stat.h>

#define MAX_COMMAND_LENGTH 10000
#define MAX_TOKENS 1000
#define MAX_TOKEN_LENGTH 1000
#define BUFLENGTH 16

// Directory paths to search for executables
#define DIR_PATHS {"/usr/local/bin", "/usr/bin", "/bin"}

// Global int variable that keeps track of if the previous command failed or succeeded
//Used for conditionals
int currstatus = 1;

typedef struct {
    int fd;
    int pos;
    int len;
    char buf[BUFLENGTH];
} lines_t;

// Function prototypes
void print_prompt();
void fdinit(lines_t *L, int fd);
char *read_command(lines_t *L); 
void parse_command(char* command, char* tokens[]);
void execute_command(char* tokens[]);
int check_wildcard(char* token, char* tokens[], int tokencount);
void execute_builtin_command(char* tokens[]);
void print_welcome_message();
void print_goodbye_message();
int check_slash(char* command);
void check_redirection(char* tokens[]);
void execute_full(char* tokens[]);
int check_pipe(char* tokens[]);
void preprocess_command(char* command);


int main(int argc, char* argv[]) {
    // Determine mode of operation (interactive or batch)
    //bool interactive_mode = false; //This was to test batch mode
    bool interactive_mode = true;
    int filefd = STDIN_FILENO;

    if (argc > 1) {
        char *filename = argv[1];
        char *lastthree = &filename[strlen(filename)-3];
        if (strcmp(lastthree, ".sh") == 0) {
            interactive_mode = false;
            filefd = open(argv[1], O_RDONLY);
        }
    } else {
        interactive_mode = isatty(STDIN_FILENO);
    }
    
    //int shfd = open(argv[1], O_RDONLY);
    lines_t inputstream;
    fdinit(&inputstream, filefd);

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
        char *line = read_command(&inputstream);
        if (line) {
            strcpy(command, line);
        } else {
            break;
        }
        
        // Parse command into tokens
        parse_command(command, tokens);

        // Execute the command
        execute_full(tokens);
    }

    // If interactive mode, print goodbye message 
    if (interactive_mode) {
        print_goodbye_message();
    }

    return 0;
}

void print_prompt() {
    const char *prompt = "mysh> ";
    write(dup(STDOUT_FILENO), prompt, strlen(prompt));
}

void print_welcome_message() {
    printf("Welcome to my shell!\n");
}

void print_goodbye_message() {
    printf("exiting\n");
}

void fdinit(lines_t *L, int fd) {
    L->fd = fd;
    L->pos = 0;
    L->len = 0;
}

char *read_command(lines_t *L) {
    // if fd isn't valid returns NULL
    if (L->fd < 0) return NULL;
    char *line = NULL;
    int line_length = 0;
    int segment_start = L->pos;

    while (1) {
        //printf("%d, %d\n", L->pos, L->len);
        if (L->pos == L->len) {
            if (segment_start < L->pos) {
                int segment_length = L->pos - segment_start;
                line = realloc(line, line_length + segment_length + 1);
                memcpy(line + line_length, L->buf + segment_start, segment_length);
                line_length = line_length + segment_length;
                line[line_length] = '\0';
            }
            L->len = read(L->fd, L->buf, BUFLENGTH);
            if (L->len < 1) {
                close(L->fd);
                L->fd = -1;
                return line;
            }
            L->pos = 0;
            segment_start = 0;
        }
        while (L->pos < L->len) {
            if (L->buf[L->pos] == '\n') {
                int segment_length = L->pos - segment_start;
                line = realloc(line, line_length + segment_length + 1);
                memcpy(line + line_length, L->buf + segment_start, segment_length);
                line[line_length + segment_length] = '\0';
                L->pos++;
                return line;
            }
            L->pos++;
        }
    }
    return NULL;
}

//Used to handle <, >, and | tokens, as they are always tokens no matter whitespace
//If they exist without white space, this function puts a space between them so we can parse it later
void preprocess_command(char* command) {
    int i = 0;
    while (command[i] != '\0') {
        if (command[i] == '>' || command[i] == '<' || command[i] == '|') {
            if (i > 0 && command[i - 1] != ' ') {
                int j = strlen(command);
                while (j >= i) {
                    command[j + 1] = command[j];
                    j--;
                }
                command[i] = ' ';
                i++;
            }
            
            if (command[i + 1] != ' ') {
                int j = strlen(command);
                while (j >= i + 1) {
                    command[j + 1] = command[j];
                    j--;
                }
                command[i + 1] = ' ';
                i++;
            }
        }
        i++;
    }
}

void parse_command(char* command, char* tokens[]) {
    char* token;
    int token_count = 0;

    //handle "< > |" edge cases
    preprocess_command(command);

    // Split the command into tokens
    token = strtok(command, " \t\n");
    while (token != NULL && token_count < MAX_TOKENS - 1) {
        int x = 0;
        //Handle when a wildcard is in the command
        if ((x = check_wildcard(token, tokens, token_count)) > 0) {
            token_count += x;
            token = strtok(NULL, " \t\n");
        } else {
            tokens[token_count] = malloc(strlen(token));
            strcpy(tokens[token_count], token);
            token_count++;
            token = strtok(NULL, " \t\n");
        }
    }
    tokens[token_count] = NULL;
}

//checks if the command is a direct pathname or not
//returns 1 if it is
int check_slash(char* command) {
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

//executes each single command based each special case
void execute_command(char* tokens[]) {
    // Save STDIN and STDOUT to handle redirection cases
    int original_stdout = dup(STDOUT_FILENO);
    int original_stdin = dup(STDIN_FILENO);

    // Check if the command has a conditional
    if (strcmp(tokens[0], "then") == 0) {
        if (currstatus != 1) {
            return;
        }
        //only run if previous command ran
        tokens = &tokens[1];
    }

    if (strcmp(tokens[0], "else") == 0) {
        //only run if previous command failed
        if (currstatus != 0) {
            return;
        }
        tokens = &tokens[1];
    }
    //if built-in command
    if (strcmp(tokens[0], "cd") == 0 || strcmp(tokens[0], "pwd") == 0 || 
        strcmp(tokens[0], "which") == 0 || strcmp(tokens[0], "exit") == 0) {
        check_redirection(tokens);
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
                    check_redirection(tokens);
                    execv(path, tokens);
                    // error if execv returns
                    perror("execv");
                    currstatus = 0;
                    exit(EXIT_FAILURE);
                } else if (pid < 0) {
                    // Fork failed
                    currstatus = 0;
                    perror("fork");
                } else {
                    // fork() returns the pid of the parent process
                    int status;
                    waitpid(pid, &status, 0);
                    currstatus = 1;
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
            check_redirection(tokens);
            execv(tokens[0], tokens);
            // error if execv returns
            perror("execv");
            currstatus = 0;
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Fork failed
            currstatus = 0;
            perror("fork");
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            currstatus = 1;
        }
    }
    dup2(original_stdout, STDOUT_FILENO);
    dup2(original_stdin, STDIN_FILENO);
    close(original_stdout);
    close(original_stdin);
}

//check if a wildcard exists and traverse the directory to find all working files that fit under the criteria
int check_wildcard(char* token, char* tokens[], int tokencount) {
    bool wildcardFound = false;
    bool pathFound = false;
    int matchCount = 0;
    char *startingpath = malloc(strlen(token));
    strcpy(startingpath, token);
    int finalPathStart = 0;
    
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '/' && i < (strlen(token)-1)) {
            finalPathStart = i+1;
            pathFound = true;
        }
    }
    
    if (pathFound) {
        startingpath[finalPathStart-1] = '\0';
    } 

    char *temptoken = malloc(strlen(token));
    strcpy(temptoken, &token[finalPathStart]);
    
    int wildcardLocation = 0;
    for (int i = 0; i < strlen(temptoken); i++) {
        if (temptoken[i] == '*') {
            wildcardFound = true;
            break;
        }
        wildcardLocation++;
    }

    if (wildcardFound) {
        // get the parts that we must check for in the wildcard
        char* firsthalf = temptoken;
        char* secondhalf = &temptoken[wildcardLocation+1];
        firsthalf[wildcardLocation] = '\0';


        DIR *d;
        struct dirent *dir;
        if (pathFound && strlen(startingpath) > 0) {
            d = opendir(startingpath);
        } else {
            d = opendir(".");
            free(startingpath);
        }
    

        if (d) {
            while ((dir = readdir(d))) {
                char *currname = dir->d_name;

                struct stat sbuf;
                
                char* fullpath = malloc(strlen(startingpath)*2+strlen(currname));

                if (pathFound) {
                    strcpy(fullpath, startingpath);
                    strcat(fullpath, "/");
                    strcat(fullpath, currname);
                } else {
                    strcpy(fullpath, currname);
                }

                int x = stat(fullpath, &sbuf);
                
                if (x < 0) {
                    closedir(d);
                    perror("Stat Error\n");
                    return matchCount;
                }
                                
                if (S_ISREG(sbuf.st_mode) && currname[0] != '.') {
                    bool matchCheck = true;

                    for (int i = 0; i < strlen(firsthalf); i++) {
                        //printf("Currname char: %c, firsthalf char: %c\n", currname[i], firsthalf[i]);
                        if ((i >= strlen(currname)) || (currname[i] != firsthalf[i])) {
                            matchCheck = false;
                        }
                    }

                    for (int i = 0; i < strlen(secondhalf); i++) {
                        //printf("Currname char: %c, secondhalf char: %c\n", currname[strlen(currname)-(i+1)], secondhalf[strlen(secondhalf)-(i+1)]);
                        if ((i >= strlen(currname)) || (currname[strlen(currname)-(i+1)] != secondhalf[strlen(secondhalf)-(i+1)])) {
                            matchCheck = false;
                        }
                    }

                    if (matchCheck && pathFound) {
                        matchCount++;
                        tokens[tokencount] = malloc(strlen(fullpath));
                        strcpy(tokens[tokencount], fullpath);
                        tokencount++;
                        tokens[tokencount] = '\0';
                    } else if (matchCheck) {
                        matchCount++;
                        tokens[tokencount] = malloc(strlen(fullpath));
                        strcpy(tokens[tokencount], fullpath);
                        tokencount++;
                        tokens[tokencount] = '\0';
                    }

                }
                free(fullpath);
            }
            closedir(d);
        }
    }
    return matchCount;
}

//free tokens for no memory leaks
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
                currstatus = 0;
                perror("cd");
            } else {
                currstatus = 1;
            }
        } else {
            currstatus = 0;
            fprintf(stderr, "cd: missing argument\n");
        }
    } else if (strcmp(tokens[0], "pwd") == 0) {
        // Print working directory
        char cwd[1000];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            currstatus = 1;
        } else {
            perror("pwd");
            currstatus = 0;
        }
    } else if (strcmp(tokens[0], "which") == 0) {
        if ((tokens[1] == NULL) || (tokens[2] != NULL) || (strcmp(tokens[1], "cd") == 0 || strcmp(tokens[1], "pwd") == 0 ||
        strcmp(tokens[1], "which") == 0 || strcmp(tokens[1], "exit") == 0)) {
            fprintf(stderr, "which: incorrect arguments\n");
            currstatus = 0;
        } else {
            char *path_env = getenv("PATH");
            char *path_env_copy = malloc(strlen(path_env)+1);
            strcpy(path_env_copy, path_env);
            char *path = strtok(path_env_copy, ":");
            free(path_env_copy);

            while (path != NULL) {
                char cmd_path[MAX_TOKEN_LENGTH*2];
                strcpy(cmd_path, path);
                strcat(cmd_path, "/");
                strcat(cmd_path, tokens[1]);

                if (access(cmd_path, X_OK) == 0) {
                    printf("%s\n", cmd_path);
                    currstatus = 1;
                    return;
                }

                path = strtok(NULL, ":");
            }

            fprintf(stderr, "which: no command found in PATH\n");
            currstatus = 0;
        }   
    } else if (strcmp(tokens[0], "exit") == 0) {
        int j = 1;
        while (j < MAX_TOKENS-1 && tokens[j] != NULL) {
            printf("%s ", tokens[j]);
            j++;
        }
        //print exit message
        printf("\nExitting mysh\n");
        free_tokens(tokens);
        currstatus = 1;
        exit(EXIT_SUCCESS);
    } 
 
}

//Checks and handles redirection
void check_redirection(char *tokens[]) {
    int i = 0;
    char *input_file = NULL;
    char *output_file = NULL;
    int input_index = -1;
    int output_index = -1; 

    // Find input and output redirection symbols
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "<") == 0) {
            //input file is the token after the symbol
            input_file = tokens[i + 1];
            //store index to remove symbol and file from the command before execution
            input_index = i; 
        } else if (strcmp(tokens[i], ">") == 0) {
            //output file is the token after the symbol
            output_file = tokens[i + 1];
            //store index to remove symbol and file from the command before execution
            output_index = i; 
        }
        i++;
    }

    // Perform input redirection 
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        //error check
        if (fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        //fd = new standard input
        dup2(fd, STDIN_FILENO);
        close(fd);

        // Remove the input redirection tokens from the array
        if (input_index >= 0) {
            tokens[input_index] = NULL;
            tokens[input_index + 1] = NULL;
        }
    }

    // Perform output redirection 
    if (output_file != NULL) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        //file descriptor = new standard output
        dup2(fd, STDOUT_FILENO);
        close(fd);

        // Remove the output redirection tokens from the array
        if (output_index >= 0) {
            tokens[output_index] = NULL;
            tokens[output_index + 1] = NULL;
        }
    }
}


//check if pipe exists in the command
//returns 1 if it does
int check_pipe(char* tokens[]) {
    int i = 0;
    int state = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "|") == 0) {
            state = 1;
            break;
        }
        i++;
    }
    return state;
}


//executes entire command and accounts for piping
//redirection takes precedence due to its usage in execute_command()
void execute_full(char* tokens[]) {

    int original_stdout = dup(STDOUT_FILENO);
    int original_stdin = dup(STDIN_FILENO);

    if (check_pipe(tokens) == 0) {
        execute_command(tokens);
    } else {
        //Traverse the tokens to find where the pipe occurs (used to split up into two processes)
        int pipe_index = 0;
        while (tokens[pipe_index] != NULL) {
            if (strcmp(tokens[pipe_index], "|") == 0) {
                break;
            }
            pipe_index++;
        }

        // Create pipe
        int p[2];
        //return error if failure
        if (pipe(p) == -1) {
            perror("pipe");
            exit(1);
        }

        // Create child 1 for first process
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("fork");
            exit(1);
        } else if (pid1 == 0) {
            // Close read end of the pipe
            close(p[0]); 
            //redirect output to the write end
            dup2(p[1], STDOUT_FILENO); 
            //close the write end to ensure no leaks
            close(p[1]);
            //set pipe token to null so execv stops when it reaches the first null token
            tokens[pipe_index] = NULL;
            // Execute the first command
            execute_command(tokens);
            wait(NULL); 
            exit(0); 
        } else {
            // Create the second process if first process is completed
            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("fork");
                exit(1);
            } else if (pid2 == 0) {
                // Close write end of the pipe
                close(p[1]); 
                //redirect input to the read end
                dup2(p[0], STDIN_FILENO); 
                 //close the read end to ensure no leaks
                close(p[0]); 

                // Execute second command
                execute_command(tokens + pipe_index + 1);
                wait(NULL); 
                exit(0); 
            } else {
                // Parent process
                close(p[0]); 
                close(p[1]);
                // Wait for both children to finish
                wait(NULL); 
                wait(NULL);
            }
        }
    }
    dup2(original_stdout, STDOUT_FILENO);
    dup2(original_stdin, STDIN_FILENO);
    close(original_stdout);
    close(original_stdin);
}
