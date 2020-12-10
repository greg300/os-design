#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFSIZE 512
#define ARGLIMIT 128

#define CMD_NORMAL 1
#define CMD_REDIRECT_OVERWRITE 2
#define CMD_REDIRECT_APPEND 3
#define CMD_PIPE 4

pid_t pid;

void resetMemory(int *commandArgCountPtr, int *errorStatusPtr, char *input, char *commandName, char *commandPath, char *commandOutputDestination)
{
    *commandArgCountPtr = 0;
    *errorStatusPtr = 0;
    strcpy(input, "\0");
    strcpy(commandName, "\0");
    strcpy(commandPath, "/bin/");
    strcpy(commandOutputDestination, "\0");
}

void executeCommand(char *currentDirectory, char *commandName, char *commandPath, char **commandArgs, int commandArgCount, int commandType, char *commandOutputDestination)
{
    int i;
    int errorStatus;
    int outputFD;

    // Prepare to launch the given command.
    // If the command is "cd", handle it separately.
    if (strcmp(commandName, "cd") == 0)
    {
        if (commandArgCount != 2)
        {
            printf("Invalid use of 'cd': 1 argument expected, %d given.\n", commandArgCount);
        }
        else
        {
            errorStatus = chdir(commandArgs[1]);
            if (errorStatus == -1)
            {
                printf("Error while using 'cd': %s.\n", strerror(errno));
            }
            else
            {
                getcwd(currentDirectory, BUFFSIZE);
            }
        }
    }
    else
    {
        strncat(commandPath, commandName, BUFFSIZE - strlen(commandPath));
        for (i = commandArgCount; i < ARGLIMIT; i++)
        {
            commandArgs[i] = NULL;
        }

        pid = fork();
        // If this is the child process, execute the command.
        if (pid == 0)
        {
            // If the output of this needs to be redirected (overwrite), do so.
            if (commandType == CMD_REDIRECT_OVERWRITE)
            {
                // Open the output file.
                outputFD = open(commandOutputDestination, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                errorStatus = outputFD;
                if (errorStatus == -1)
                {
                    printf("Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // Set the outputFD to be the file descriptor of stdout.
                errorStatus = dup2(outputFD, STDOUT_FILENO);
                if (errorStatus == -1)
                {
                    printf("Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }
            }
            // If the output of this needs to be redirected (append), do so.
            else if (commandType == CMD_REDIRECT_APPEND)
            {
                // Open the output file.
                outputFD = open(commandOutputDestination, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
                errorStatus = outputFD;
                if (errorStatus == -1)
                {
                    printf("Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // Set the outputFD to be the file descriptor of stdout.
                errorStatus = dup2(outputFD, STDOUT_FILENO);
                if (errorStatus == -1)
                {
                    printf("Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }
            }
            // If the output of this needs to be piped, do so.
            else if (commandType == CMD_PIPE)
            {
                // Pipe stdout to a fd for retrieval by parent.
            }

            //fprintf(stdout, "Executing command %s.\n", commandPath);
            // Execute the command.
            errorStatus = execvp(commandPath, commandArgs);
            if (errorStatus == -1)
            {
                fprintf(stdout, "Error while executing command: %s.\n", strerror(errno));
            }
            exit(0);
        }
        
        waitpid(pid, NULL, 0);
        //printf("Done\n");
    }
    
    // Deallocate the command arguments.
    for (i = 0; i < commandArgCount; i++)
    {
        free(commandArgs[i]);
    }
}

int main()
{
    int i;
    int errorStatus;
    int commandArgCount = 0;
    int tokenCount = 0;
    char *input = malloc(BUFFSIZE * sizeof(char));
    char *currentDirectory = malloc(BUFFSIZE * sizeof(char));
    char *token;
    char *commandName = malloc(BUFFSIZE * sizeof(char));
    char *commandPath = malloc(BUFFSIZE * sizeof(char));
    char *commandOutputDestination = malloc(BUFFSIZE * sizeof(char));
    char **commandArgs = malloc(ARGLIMIT * sizeof(char *));
    for (i = 0; i < ARGLIMIT; i++)
    {
        commandArgs[i] = malloc(BUFFSIZE * sizeof(char));
    }
    
    getcwd(currentDirectory, BUFFSIZE);

    while (strcmp(input, "exit") != 0)
    {
        resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
        tokenCount = 0;

        // Prompt the user for their input command(s).
        printf("%s $ ", currentDirectory);
        fgets(input, BUFFSIZE, stdin);
        
        // Remove the trailing newline.
        char *newLinePos = strchr(input, '\n');
        if (newLinePos != NULL)
        {
            *newLinePos = '\0';
        }

        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        //printf("Input: %s\n", input);

        // Get the first token.
        token = strtok(input, " ");
        while (token != NULL)
        {
            //printf("Token: %s\n", token);
            // Interpret this token.
            if (commandArgCount == 0)
            {
                strncpy(commandName, token, BUFFSIZE);
            }

            // If token is a semicolon, prepare to launch the built command.
            if (strcmp(token, ";") == 0)
            {
                // Execute the command.
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_NORMAL, NULL);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
            }
            // If token is a ">", get the next token and prepare to launch the built command and redirect the output to a file (overwrite).
            else if (strcmp(token, ">") == 0)
            {
                // Get the next token.
                token = strtok(NULL, " ");
                tokenCount++;

                // This token should be the file into which output of the built command will be sent.
                strcpy(commandOutputDestination, token);

                // Execute the command.
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_REDIRECT_OVERWRITE, commandOutputDestination);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
            }
            // If token is a ">>", get the next token and prepare to launch the built command and redirect the output to a file (append).
            else if (strcmp(token, ">>") == 0)
            {
                // Get the next token.
                token = strtok(NULL, " ");
                tokenCount++;

                // This token should be the file into which output of the built command will be sent.
                strcpy(commandOutputDestination, token);

                // Execute the command.
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_REDIRECT_APPEND, commandOutputDestination);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
            }
            // If token is a "|", prepare to launch the built command and pipe the output to the next command.
            else if (strcmp(token, "|") == 0)
            {
                // Execute the command.
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_PIPE, NULL);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
            }
            // Otherwise, token is a command or command argument.
            else
            {
                // Allocate space for a new command argument.
                commandArgs[commandArgCount] = malloc(BUFFSIZE * sizeof(char));
                // Copy the argument to the list of arguments.
                strncpy(commandArgs[commandArgCount], token, BUFFSIZE);
                // Increment the number of arguments and tokens read for this command.
                commandArgCount++;
            }

            // Get the next token.
            token = strtok(NULL, " ");
            tokenCount++;
        }

        // Prepare to launch this command.
        if (strcmp(commandName, "\0") != 0)
        {
            executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_NORMAL, NULL);
        }
    }

    // Deallocate all memroy used.
    free(input);
    free(currentDirectory);
    free(commandName);
    free(commandPath);
    free(commandOutputDestination);
    free(commandArgs);

    return 0;
}