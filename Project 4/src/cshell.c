#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>

#define BUFFSIZE 512
#define ARGLIMIT 128

int main()
{
    int i;
    int errorStatus;
    int commandArgCount = 0;
    int tokenCount = 0;
    pid_t pid;
    char *input = malloc(BUFFSIZE * sizeof(char));
    char *currentDirectory = malloc(BUFFSIZE * sizeof(char));
    char *token;
    char *commandName = malloc(BUFFSIZE * sizeof(char));
    char *commandPath = malloc(BUFFSIZE * sizeof(char));
    char **commandArgs = malloc(ARGLIMIT * sizeof(char *));
    for (i = 0; i < ARGLIMIT; i++)
    {
        commandArgs[i] = malloc(BUFFSIZE * sizeof(char));
    }
    
    getcwd(currentDirectory, BUFFSIZE);

    while (strcmp(input, "exit") != 0)
    {
        commandArgCount = 0;
        tokenCount = 0;
        errorStatus = 0;
        strcpy(input, "\0");
        strcpy(commandName, "\0");
        strcpy(commandPath, "/bin/");
        // for (i = 0; i < ARGLIMIT; i++)
        // {
        //     //commandArgs[i] = "\0";
        //     strcpy(commandArgs[i], "\0");
        // }

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
            if (tokenCount == 0)
            {
                strncpy(commandName, token, BUFFSIZE);
            }
            else
            {
                // If token is a semicolon, prepare to launch the built command.


            }
            commandArgs[commandArgCount] = malloc(BUFFSIZE * sizeof(char));
            strncpy(commandArgs[commandArgCount], token, BUFFSIZE);
            commandArgCount++;
            tokenCount++;

            // Get the next token.
            token = strtok(NULL, " ");
        }

        // Prepare to launch this command.
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
            if (pid == 0)
            {
                //printf("Executing command %s.\n", commandPath);
                execvp(commandPath, commandArgs);

                // Pipe stdout to a fd for retrieval by parent.
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

    // Deallocate all memroy used.
    free(input);
    free(currentDirectory);
    free(commandName);
    free(commandPath);
    // for (i = 0; i < ARGLIMIT; i++)
    // {
    //     free(commandArgs[i]);
    // }
    free(commandArgs);

    return 0;
}