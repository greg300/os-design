#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFSIZE 4*1024
#define ARGLIMIT 128
#define PIPELIMIT 128

#define CMD_NORMAL 1
#define CMD_REDIRECT_OVERWRITE 2
#define CMD_REDIRECT_APPEND 3
#define CMD_PIPE 4

pid_t pid;
int allPipeFDs[PIPELIMIT][2];
int pipeLoaded = 0;
int restartParse = 0;

void interruptSignalHandler(int sigNum)
{
	//printf("Caught signal %d.\n", sigNum);
    int errorStatus;
    printf("\n");

    // If there is a process running (pid > 0), kill it.
    if (pid > 0)
    {
        errorStatus = kill(pid, SIGKILL);
        if (errorStatus == -1)
        {
            printf("Error while killing process %d: %s.\n", pid, strerror(errno));
        }
        restartParse = 1;
    }
}

void resetMemory(int *commandArgCountPtr, int *errorStatusPtr, char *input, char *commandName, char *commandPath,
                 char *commandOutputDestination)
{
    *commandArgCountPtr = 0;
    *errorStatusPtr = 0;
    strcpy(input, "\0");
    strcpy(commandName, "\0");
    strcpy(commandPath, "\0");
    strcpy(commandOutputDestination, "\0");
}

void executeCommand(char *currentDirectory, char *commandName, char *commandPath, char **commandArgs,
                    int commandArgCount, int commandType, char *commandOutputDestination, int pipesCount)
{
    int i;
    int errorStatus;
    int outputFD;

    if (commandType == CMD_PIPE)
    {
        errorStatus = pipe(allPipeFDs[pipesCount]);
        if (errorStatus == -1)
        {
            printf("Error while piping output: %s.\n", strerror(errno));
            return;
        }
    }

    // Prepare to launch the given command.
    // If the command is "cd", handle it separately.
    if (strcmp(commandName, "cd") == 0)
    {
        // Check for invalid argument case.
        if (commandArgCount != 2)
        {
            printf("Invalid use of 'cd': 1 argument expected, %d given.\n", commandArgCount);
        }
        else
        {
            // Change directory using chdir.
            errorStatus = chdir(commandArgs[1]);
            if (errorStatus == -1)
            {
                printf("Error while using 'cd': %s.\n", strerror(errno));
            }
            else
            {
                // Update the current directory.
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
            // If the previous command had its output piped to this command, pipe it.
            if (pipeLoaded == 1 && pipesCount > 0)
            {
                //printf("Unloading pipe with pipesCount = %d.\n", pipesCount);
                // Close the write end of the pipe.
                close(allPipeFDs[pipesCount - 1][1]);

                // char buf[100];
                // read(allPipeFDs[pipesCount - 1][0], buf, 99);
                // printf("%s\n", buf);

                // Pipe stdout from fd to the stdin of the new process.
                // Send STDIN to the pipe.
                dup2(allPipeFDs[pipesCount - 1][0], STDIN_FILENO);

                // No further action needed from this descriptor.
                close(allPipeFDs[pipesCount - 1][0]);
            }

            // If the output of this needs to be redirected (overwrite) (>), do so.
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

                // No further action needed from this descriptor.
                close(outputFD);
            }
            // If the output of this needs to be redirected (append) (>>), do so.
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

                // No further action needed from this descriptor.
                close(outputFD);
            }
            // If the output of this needs to be piped (|), do so.
            else if (commandType == CMD_PIPE)
            {
                //printf("Loading pipe with pipesCount = %d.\n", pipesCount);
                // Pipe stdout to a fd for retrieval by parent.
                // Close the reading end for the child.
                close(allPipeFDs[pipesCount][0]);

                // Send STDOUT to the pipe.
                dup2(allPipeFDs[pipesCount][1], STDOUT_FILENO);
                //dup2(pipeFD[1], STDERR_FILENO);

                // No further action needed from this descriptor.
                close(allPipeFDs[pipesCount][1]);
            }

            //fprintf(stderr, "Executing command %s.\n", commandPath);
            // Execute the command.
            errorStatus = execvp(commandPath, commandArgs);

            if (errorStatus == -1)
            {
                // If the command is not found in the default /bin, try in /usr/bin.
                // if (errno == EM)
                // strncat(commandPath, commandName, BUFFSIZE - strlen(commandPath));
                // errorStatus = execvp(commandPath, commandArgs);
                if (errorStatus == -1)
                {
                    fprintf(stderr, "Error while executing command: %s.\n", strerror(errno));
                }
                exit(0);
            }
        }
        // If this is the parent process, wait for the child to finish executing its command.
        else if (pid > 0)
        {
            if (pipeLoaded == 1)
            {
                close(allPipeFDs[pipesCount - 1][0]);
                close(allPipeFDs[pipesCount - 1][1]);
            }
            waitpid(pid, NULL, 0);

            // Pipe is "unloaded", if it had been loaded.
            pipeLoaded = 0;
        }
        else
        {
            fprintf(stderr, "Error while creating process for command: %s.\n", strerror(errno));
        }
        
        pid = -1;
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
    int pipesCount = 0;
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

    // Create the signal handler for the interrupt signal, SIGINT.
    struct sigaction interruptAction;
	interruptAction.sa_handler = interruptSignalHandler;
	sigemptyset(&interruptAction.sa_mask);
	interruptAction.sa_flags = 0;

    // Define the action upon interrupt.
	sigaction(SIGINT, &interruptAction, NULL);
    
    getcwd(currentDirectory, BUFFSIZE);

    while (strcmp(input, "exit") != 0)
    {
        resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
        // Reset tokenCount and pipesCount separately, since we only want to reset these once per input.
        tokenCount = 0;
        pipesCount = 0;

        // Make sure we do not continually restart the parsing, if we came here from a SIGINT killing the last command.
        restartParse = 0;

        // Prompt the user for their input command(s).
        printf("%s $ ", currentDirectory);
        fgets(input, BUFFSIZE, stdin);
        
        // Remove the trailing newline.
        char *newLinePos = strchr(input, '\n');
        if (newLinePos != NULL)
        {
            *newLinePos = '\0';
        }

        // If the user entered "exit", stop the CShell.
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        //printf("Input: %s\n", input);

        // Get the first token.
        token = strtok(input, " ");

        while (token != NULL)
        {
            if (restartParse == 1)
            {
                break;
            }
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
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_NORMAL, NULL, pipesCount);

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
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_REDIRECT_OVERWRITE, commandOutputDestination, pipesCount);

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
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_REDIRECT_APPEND, commandOutputDestination, pipesCount);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);
            }
            // If token is a "|", prepare to launch the built command and pipe the output to the next command.
            else if (strcmp(token, "|") == 0)
            {
                // Execute the command.
                executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_PIPE, NULL, pipesCount);

                // Reset counters and memory for the next command.
                resetMemory(&commandArgCount, &errorStatus, input, commandName, commandPath, commandOutputDestination);

                // Increment number of pipes for commands that follow in this input.
                pipesCount++;

                // Pipe is "loaded".
                pipeLoaded = 1;
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
        if (restartParse == 1)
        {
            continue;
        }

        // Prepare to launch this command, if there is one.
        if (strcmp(commandName, "\0") != 0)
        {
            executeCommand(currentDirectory, commandName, commandPath, commandArgs, commandArgCount, CMD_NORMAL, NULL, pipesCount);
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