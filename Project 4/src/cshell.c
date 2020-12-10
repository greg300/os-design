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

#define BUFFSIZE 4*1024                 // The maximum number of bytes that one line of input can be.
#define ARGLIMIT 128                    // The maximum number of arguments for a single command.
#define PIPELIMIT 128                   // The maximum number of pipes for one line of input.

#define CMD_NORMAL 1                    // Indicates a command to be executed without redirection or piping.
#define CMD_REDIRECT_OVERWRITE 2        // Indicates a command to be executed with output redirection in overwrite mode.
#define CMD_REDIRECT_APPEND 3           // Indicates a command to be executed with output redirection in append mode.
#define CMD_PIPE 4                      // Indicates a command to be executed with piping (piping its output to a pipe).

pid_t pid;                              // The currently-running child process/command.
int allPipeFDs[PIPELIMIT][2];           // Array of all read/write file descriptors for all currently-existing pipes.
int pipeLoaded = 0;                     // Determines whether the pipe is currently loaded (has output from previous command).
int restartParse = 0;                   // Determines whether parsing ought to be restarted (following an interrupt signal).


/*
[SIGNAL HANDLER] Catches the interrupt signal (ctrl+c) and kills the currently-running child process, if any.
*/
void interruptSignalHandler(int sigNum)
{
    int errorStatus;

    // Print a newline for cleanliness.
    printf("\n");

    // If there is a process running (pid != -1), kill it.
    if (pid > 0)
    {
        // Send SIGKILL to the process.
        errorStatus = kill(pid, SIGKILL);
        if (errorStatus == -1)
        {
            fprintf(stderr, "Error while killing process %d: %s.\n", pid, strerror(errno));
        }

        // Ensure that input parsing does not continue, so that any other commands in the input line are not executed.
        restartParse = 1;
    }
}


/*
[HELPER] Resets some key variables to prepare for execution of next command.
*/
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


/*
[HELPER] Deallocates any allocated command arguments.
*/
void deallocateCommandArgs(char **commandArgs, int commandArgCount)
{
    int i;

    // Deallocate the command arguments.
    for (i = 0; i < commandArgCount; i++)
    {
        free(commandArgs[i]);
    }
}


/*
[HELPER] Executes the command given by commandName.
*/
void executeCommand(char *currentDirectory, char *commandName, char *commandPath, char **commandArgs,
                    int commandArgCount, int commandType, char *commandOutputDestination, int pipesCount)
{
    int i;
    int errorStatus;
    int outputFD;

    // If this command needs to have its output piped to the next command, create the pipe as the parent.
    if (commandType == CMD_PIPE)
    {
        // Create a pipe, and save it to the array of pipes.
        errorStatus = pipe(allPipeFDs[pipesCount]);
        if (errorStatus == -1)
        {
            fprintf(stderr, "Error while piping output: %s.\n", strerror(errno));
            deallocateCommandArgs(commandArgs, commandArgCount);
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
            fprintf(stderr, "Invalid use of 'cd': 1 argument expected, %d given.\n", commandArgCount);
        }
        else
        {
            // Change directory using chdir.
            errorStatus = chdir(commandArgs[1]);
            if (errorStatus == -1)
            {
                fprintf(stderr, "Error while using 'cd': %s.\n", strerror(errno));
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
        // The commandPath should be the commandName, which is also commandArgs[0].
        strncat(commandPath, commandName, BUFFSIZE - strlen(commandPath));

        // Make sure all unused command arguments are NULL.
        for (i = commandArgCount; i < ARGLIMIT; i++)
        {
            commandArgs[i] = NULL;
        }

        // Fork the child process that will run this command.
        pid = fork();

        // If this is the child process, execute the command.
        if (pid == 0)
        {
            // First, check if the pipe is loaded.
            // If the previous command had its output piped to this command, pipe it.
            if (pipeLoaded == 1 && pipesCount > 0)
            {
                // Close the write end of the pipe (child's write end).
                close(allPipeFDs[pipesCount - 1][1]);

                // Pipe STDOUT from the previous command to the STDIN of the new command.
                dup2(allPipeFDs[pipesCount - 1][0], STDIN_FILENO);

                // No further action needed from this descriptor (child's read end).
                close(allPipeFDs[pipesCount - 1][0]);
            }

            // If the output of this needs to be redirected (overwrite) (>), do so.
            if (commandType == CMD_REDIRECT_OVERWRITE)
            {
                // Open the output file (with the O_TRUNC flag to overwrite).
                outputFD = open(commandOutputDestination, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                errorStatus = outputFD;
                if (errorStatus == -1)
                {
                    fprintf(stderr, "Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // Set the outputFD to be the file descriptor of stdout.
                errorStatus = dup2(outputFD, STDOUT_FILENO);
                if (errorStatus == -1)
                {
                    fprintf(stderr, "Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // No further action needed from this descriptor.
                close(outputFD);
            }
            // If the output of this needs to be redirected (append) (>>), do so.
            else if (commandType == CMD_REDIRECT_APPEND)
            {
                // Open the output file (with the O_APPEND flag to append).
                outputFD = open(commandOutputDestination, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
                errorStatus = outputFD;
                if (errorStatus == -1)
                {
                    fprintf(stderr, "Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // Set the outputFD to be the file descriptor of stdout.
                errorStatus = dup2(outputFD, STDOUT_FILENO);
                if (errorStatus == -1)
                {
                    fprintf(stderr, "Error redirecting output: %s.\n", strerror(errno));
                    exit(0);
                }

                // No further action needed from this descriptor.
                close(outputFD);
            }
            // If the output of this needs to be piped (|), do so.
            else if (commandType == CMD_PIPE)
            {
                // Close the read end of the pipe (child's read end).
                close(allPipeFDs[pipesCount][0]);

                // Pipe STDOUT from this command to what will become the STDIN of the new command.
                dup2(allPipeFDs[pipesCount][1], STDOUT_FILENO);

                // No further action needed from this descriptor (child's write end).
                close(allPipeFDs[pipesCount][1]);
            }

            // Execute the command.
            errorStatus = execvp(commandPath, commandArgs);
            if (errorStatus == -1)
            {
                fprintf(stderr, "Error while executing command: %s.\n", strerror(errno));
                exit(0);
            }
        }
        // If this is the parent process, wait for the child to finish executing its command.
        else if (pid > 0)
        {
            // If the pipe was loaded, close the parent's read and write file descriptors for the loaded pipe.
            if (pipeLoaded == 1)
            {
                close(allPipeFDs[pipesCount - 1][0]);
                close(allPipeFDs[pipesCount - 1][1]);
            }
            
            // Wait on the child.
            waitpid(pid, NULL, 0);

            // Pipe is "unloaded", if it had been loaded.
            pipeLoaded = 0;
        }
        else
        {
            fprintf(stderr, "Error while creating process for command: %s.\n", strerror(errno));
        }
        
        pid = -1;
    }
    
    // Deallocate the command arguments (should be reached by "cd" logic as well, regardless of success).
    deallocateCommandArgs(commandArgs, commandArgCount);
}


/*
[MAIN] Starts the shell and parses user input.
*/
int main()
{
    int errorStatus;                                                        // Error status of a system call.
    int commandArgCount = 0;                                                // Number of command arguments parsed for a given input.
    int tokenCount = 0;                                                     // Number of tokens parsed for a given input.
    int pipesCount = 0;                                                     // Number of pipes parsed for a given input.
    char *token;                                                            // Current token (delimited by a space " ").
    char *input = malloc(BUFFSIZE * sizeof(char));                          // Current line of input.
    char *currentDirectory = malloc(BUFFSIZE * sizeof(char));               // Current working directory.
    char *commandName = malloc(BUFFSIZE * sizeof(char));                    // Current name of the command to be run.
    char *commandPath = malloc(BUFFSIZE * sizeof(char));                    // Current path to the command to be run.
    char *commandOutputDestination = malloc(BUFFSIZE * sizeof(char));       // Current destination for output redirection (if applicable).
    char **commandArgs = malloc(ARGLIMIT * sizeof(char *));                 // Current arguments for the command to be run.

    // Create the signal handler for the interrupt signal, SIGINT.
    struct sigaction interruptAction;
	interruptAction.sa_handler = interruptSignalHandler;
	sigemptyset(&interruptAction.sa_mask);
	interruptAction.sa_flags = 0;

    // Define the action upon interrupt.
	sigaction(SIGINT, &interruptAction, NULL);
    
    // Get the current working directory.
    getcwd(currentDirectory, BUFFSIZE);

    // Continue reading user input until the user quits the shell using "exit".
    while (strcmp(input, "exit") != 0)
    {
        // Reset some key variables to prepare for execution of next command.
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

        // Get the first token.
        token = strtok(input, " ");

        // Interpret this and succeeding tokens.
        while (token != NULL)
        {
            // If we came here from an interrupt, restart the parse.
            if (restartParse == 1)
            {
                break;
            }

            // If this is the first argument, it should be the commandName.
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
                strncpy(commandOutputDestination, token, BUFFSIZE);

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
                strncpy(commandOutputDestination, token, BUFFSIZE);

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

                // Increment the number of arguments read for this command.
                commandArgCount++;
            }

            // Get the next token.
            token = strtok(NULL, " ");
            tokenCount++;
        }

        // If we came here from an interrupt, restart the parse.
        if (restartParse == 1)
        {
            continue;
        }

        // We have reached the end of our input.
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