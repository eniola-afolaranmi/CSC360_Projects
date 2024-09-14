#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

struct bg_pro
{
    pid_t pid;
    char *command;
    struct bg_pro *next;
};

struct bg_pro *root = NULL; //initialize linked list root and count
int bg_num = 0;

void addToList(pid_t pid, char *argv[])

{
    int i;
    size_t length = 0;
    for (i = 0; argv[i] != NULL; i++)
    {
        length += strlen(argv[i]) + 1; // plus 1 representing spaces
    }

    char *command = (char *)malloc(length);
    if (command == NULL)
    {
        perror("malloc");
        exit(1);
    }

    command[0] = '\0'; // Initialize the string

    for (i = 0; argv[i] != NULL; i++)
    {
        strcat(command, argv[i]);
        strcat(command, " ");
    }

    struct bg_pro *bg_process = malloc(sizeof(struct bg_pro));
    if (bg_process == NULL)
    {
        exit(1);
    }

    bg_process->pid = pid;
    bg_process->command = command;
    bg_process->next = NULL;

    if (root == NULL)
    {
        root = bg_process;
    }
    else
    {
        struct bg_pro *cur = root;
        while (cur->next != NULL)
        {
            cur = cur->next;
        }
        cur->next = bg_process;
    }
    bg_num++; //increase number of background processes
}

char  *createCommandPrompt()
{
    char *login;
    char hostname[256];
    char path[256];

    login = getlogin();
    gethostname(hostname, sizeof(hostname));
    getcwd(path, sizeof(path));

    char *commandPrompt = (char *)malloc(1024);

    snprintf(commandPrompt, 517, "%s@%s: %s > ", login, hostname, path);
    return commandPrompt;
}

void printBgList()
{
    printf("Background Processes: \n");
    if (bg_num == 0)
    {
        printf("There are no processes running in the background\n");
    }
    else
    {
        printf("There are %d processes running in the background\n", bg_num);
        struct bg_pro *cur = root;
        while (cur != NULL)
        {
            printf("%d: %s\n", cur->pid, cur->command);
            cur = cur->next;
        }
    }
}

void checkBackgroundProcesses()
{
    if (bg_num > 0)
    {
        pid_t ter = waitpid(0, NULL, WNOHANG); // if there is a terminated process

        if (ter > 0)
        {
            struct bg_pro *cur = root;
            if (root->pid == ter)
            {
                printf("\n%d: %s has terminated\n", root->pid, root->command);
                cur = root;
                root = root->next;

                free(cur->command); //free removed node memory
                free(cur);
            }
            else
            {
                while (cur->next->pid != ter)
                {
                    cur = cur->next;
                }
                printf("\n%d: %s has terminated\n\n", cur->pid, cur->command);
                cur->next = cur->next->next;

                free(cur->next->command);
                free(cur->next);
            }
            bg_num--; //remove node from bg_pro count
        }
    }
}


void cdCommand(char *argv[])
{
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0) //taken from tutorial
    {
        chdir(getenv("HOME"));
    }
    else
    {
        if (chdir(argv[1]) != 0)
        {
            perror("chdir");
        }
    }
}

void bgCommand(char *argv[])
{
    int i;
    char *newArgv[256];
    for (i = 0; argv[i + 1] != NULL; i++)
    {
        newArgv[i] = argv[i + 1];
    }
    newArgv[i] = NULL;

    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(newArgv[0], newArgv);
        perror("execvp");
        exit(1);
    }
    else if (pid > 0)
    {
        addToList(pid, &newArgv[0]);
    }
    else
    {
        perror("fork");
    }
}

void shellCommand(char *argv[])
{
    pid_t p = fork();
    if (p == 0)
    {
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }
    else if (p == -1)
    {
        printf("Child was not able to be made\n");
    }
    else
    {
        pid_t wait = waitpid(p, NULL, 0);
    }
}
int main()
{

    int bailout = 0;
    char *argv[256];
    char *prompt;

    while (!bailout)
    {
        prompt = createCommandPrompt();
        char *reply = readline(prompt);
        checkBackgroundProcesses(); // check for any terminated background processes

        if (strcmp(reply, "exit") == 0)
        {
            bailout = 1;
            free(reply);
            continue;
        }
        else
        {
            char *token = strtok(reply, " \t\n");
            int i = 0;
            while (token != NULL)
            {
                argv[i++] = token;
                token = strtok(NULL, " \t\n");
            }
            argv[i] = NULL;

            if (strcmp(argv[0], "cd") == 0)
            {
                cdCommand(argv);
            }
            else if (strcmp(argv[0], "bg") == 0)
            {
                bgCommand(argv);
            }
            else if (strcmp(argv[0], "bglist") == 0)
            {
                printBgList();
            }
            else
            {
                shellCommand(argv);
            }
        }
        free(reply);
        free(prompt);
    }

    struct bg_pro *cur = root;
    while (root != NULL)
    {
        cur = root;
        root = root->next;
        free(cur->command);
        free(cur);
    }
    printf("Bye Bye\n");
}
