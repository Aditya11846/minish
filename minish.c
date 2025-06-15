#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 100

void handle_sigint(int sig) {
    printf("\nCaught SIGINT (Ctrl+C).\n");
    printf("minish> ");
    fflush(stdout);
}

// Simple tokenizer that splits into argv-style args
void parse_input(char* input, char** args) {
    input[strcspn(input, "\n")] = '\0'; // strip newline
    char* token = strtok(input, " ");
    int i = 0;
    while (token && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

// Executes command with support for redirection, pipes, and background
void execute_command(char** args) {
    if (args[0] == NULL) return;

    // Handle 'exit'
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // Handle 'cd'
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "minish: expected argument to \"cd\"\n");
        } else if (chdir(args[1]) != 0) {
            perror("minish");
        }
        return;
    }

    // Check for background execution
    int background = 0;
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;
            break;
        }
    }

    // Check for pipes
    int pipe_index = -1;
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1) {
        args[pipe_index] = NULL;
        char* left_args[MAX_ARGS], *right_args[MAX_ARGS];

        for (int i = 0; i < pipe_index; i++) left_args[i] = args[i];
        left_args[pipe_index] = NULL;

        for (int i = pipe_index + 1, j = 0; args[i]; i++, j++) right_args[j] = args[i];
        right_args[MAX_ARGS - 1] = NULL;

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            signal(SIGINT, SIG_DFL);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]); close(pipefd[1]);
            execvp(left_args[0], left_args);
            perror("execvp left");
            exit(1);
        }

        pid_t pid2 = fork();
        if (pid2 == 0) {
            signal(SIGINT, SIG_DFL);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]); close(pipefd[0]);
            execvp(right_args[0], right_args);
            perror("execvp right");
            exit(1);
        }

        close(pipefd[0]); close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return;
    }

    // Check for redirection
    int in_redirect = -1, out_redirect = -1, append_redirect = -1;
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) in_redirect = i;
        else if (strcmp(args[i], ">>") == 0) append_redirect = i;
        else if (strcmp(args[i], ">") == 0) out_redirect = i;
    }

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);

        if (in_redirect != -1) {
            int fd = open(args[in_redirect + 1], O_RDONLY);
            if (fd < 0) { perror("input redirection"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[in_redirect] = NULL;
        }

        if (out_redirect != -1) {
            int fd = open(args[out_redirect + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("output redirection"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[out_redirect] = NULL;
        }

        if (append_redirect != -1) {
            int fd = open(args[append_redirect + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror("append redirection"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[append_redirect] = NULL;
        }

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        if (!background) waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
}

void run_shell() {
    char input[MAX_CMD_LEN];
    char* args[MAX_ARGS];

    signal(SIGINT, handle_sigint);

    while (1) {
        printf("minish> ");
        fflush(stdout);
        if (!fgets(input, MAX_CMD_LEN, stdin)) break;
        parse_input(input, args);
        execute_command(args);
    }
}

int main() {
    run_shell();
    return 0;
}
