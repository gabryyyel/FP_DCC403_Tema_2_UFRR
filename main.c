#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"

/* =========================================
   1. TRATAMENTO DE SINAIS (ANTI-ZOMBIE)
   ========================================= */
void sigchld_handler(int signo) {
    (void)signo; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* =========================================
   2. PARSING E EXPANSÃO (GLOB)
   ========================================= */
char **split_line(char *line) {
    int bufsize = TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "edge-engine: erro de alocação\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOK_DELIM);
    while (token != NULL) {
        tokens[position++] = token;
        if (position >= bufsize) {
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "edge-engine: erro de realocação\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, TOK_DELIM);
    }
    tokens[position] = NULL; 
    return tokens;
}

void free_expanded_args(char **args) {
    if (!args) return;
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

char **expand_wildcards(char **args) {
    int bufsize = 64;
    int position = 0;
    char **expanded_args = malloc(bufsize * sizeof(char*));

    if (!expanded_args) {
        perror("edge-engine: erro de alocação no glob");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; args[i] != NULL; i++) {
        if (strchr(args[i], '*') != NULL || strchr(args[i], '?') != NULL) {
            glob_t glob_result;
            int ret = glob(args[i], GLOB_NOCHECK, NULL, &glob_result);

            if (ret == 0) {
                for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                    expanded_args[position++] = strdup(glob_result.gl_pathv[j]);
                    if (position >= bufsize) {
                        bufsize += 64;
                        expanded_args = realloc(expanded_args, bufsize * sizeof(char*));
                    }
                }
            } else {
                expanded_args[position++] = strdup(args[i]);
            }
            globfree(&glob_result); 
        } else {
            expanded_args[position++] = strdup(args[i]);
        }

        if (position >= bufsize) {
            bufsize += 64;
            expanded_args = realloc(expanded_args, bufsize * sizeof(char*));
        }
    }
    
    expanded_args[position] = NULL;
    return expanded_args;
}

/* =========================================
   3. EXECUÇÃO DE PROCESSOS (FORK E EXEC)
   ========================================= */
int launch_process(char **args) {
    pid_t pid;
    int background = 0;
    int i = 0;

    while (args[i] != NULL) i++;

    if (i > 0 && strcmp(args[i-1], "&") == 0) {
        background = 1;
        args[i-1] = NULL; 
    }

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("edge-engine");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("edge-engine: erro no fork");
    } else {
        if (!background) {
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        } else {
            printf("[Processo assíncrono iniciado com PID %d]\n", pid);
        }
    }
    return 1;
}

int launch_piped_process(char **cmd1, char **cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;
    int status;

    if (pipe(pipefd) == -1) {
        perror("edge-engine: falha fatal na criação do pipe");
        return 1;
    }

    pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); 
        if (execvp(cmd1[0], cmd1) == -1) {
            perror("edge-engine: falha cmd1");
            exit(EXIT_FAILURE);
        }
    }

    pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]); 
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]); 
        if (execvp(cmd2[0], cmd2) == -1) {
            perror("edge-engine: falha cmd2");
            exit(EXIT_FAILURE);
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, &status, WUNTRACED);
    waitpid(pid2, &status, WUNTRACED);

    return 1;
}

/* =========================================
   4. LOOP PRINCIPAL (MODO INTERATIVO)
   ========================================= */
void print_prompt() {
    printf("edge-engine> ");
    fflush(stdout); 
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; 

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("edge-engine: erro fatal ao registrar sigaction");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while (1) {
        print_prompt();
        read_bytes = getline(&line, &len, stdin);

        if (read_bytes == -1) {
            printf("\nEncerrando orquestrador...\n");
            break;
        }

        if (read_bytes > 0 && line[read_bytes - 1] == '\n') {
            line[read_bytes - 1] = '\0';
        }

        if (strlen(line) == 0) continue;

        char **raw_args = split_line(line);
        if (raw_args[0] == NULL) {
            free(raw_args);
            continue;
        }

        if (strcmp(raw_args[0], "exit") == 0) {
            free(raw_args);
            break;
        }

        char **expanded_args = expand_wildcards(raw_args);

        // Verifica se há encadeamento por Pipe
        int pipe_idx = -1;
        for (int i = 0; expanded_args[i] != NULL; i++) {
            if (strcmp(expanded_args[i], "|") == 0) {
                pipe_idx = i;
                break;
            }
        }

        // Roteamento de Execução
        if (pipe_idx != -1) {
            expanded_args[pipe_idx] = NULL; // Quebra a string no meio
            char **cmd1 = expanded_args;
            char **cmd2 = &expanded_args[pipe_idx + 1];
            launch_piped_process(cmd1, cmd2);
        } else {
            launch_process(expanded_args);
        }

        free_expanded_args(expanded_args); 
        free(raw_args);                    
    } 

    free(line);
    return 0;
}