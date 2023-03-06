#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Needed libaries for working with process */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* File system control library */
#include "parse_token.h"
#include "vect.h"
#include <fcntl.h>

#define DYNAMIC_SIZE 2
#define CMD_DATA_SIZE sizeof(char *)

/* Shell special commands */
#define OUT_DIRECTION_CMD ">"
#define IN_DIRECTION_CMD "<"
#define SEQUENCE_CMD ";"
#define PIPE_CMD "|"

#define WELCOME_MSG "Welcome to mini-shell.\n"
#define SHELL_PROMPT_MSG "shell $ "
#define EXIT_MSG "\nBye bye.\n"
#define INVALID_COMMAND_MSG "%s: command not found"

typedef struct state_machine state_machine_t;
typedef struct command command_t;

/* @command: Command implementation */
struct command {
  char *token;
  char **args;
};

void process_script(char *filename);

/* @command: Initialize new command */
command_t *init_command(char *token, int csize) {
  command_t *c = (command_t *)malloc(sizeof(command_t));
  c->args = (char **)malloc(sizeof(char *) * csize);
  c->token = (char *)malloc(sizeof(char) * (strlen(token) + 1));
  return c;
}

/* @command: Run compaction on commands */
command_t *compact_command(char **commands, int csize) {
  int command_size = sizeof(char) * (strlen(commands[0]) + 1);
  command_t *c = init_command(commands[0], csize);
  strcpy(c->token, commands[0]);
  for (int i = 0; i < csize; i++) {
    int l = strlen(commands[i]);
    if (l > 0) {
      int s = sizeof(char) * l + 1;
      c->args[i] = (char *)malloc(s);
      strcpy(c->args[i], commands[i]);
    }
  }
  c->args[csize] = NULL;
  return c;
}

/* @state_machine: State machine implementation */
struct state_machine {
  char *token_state;
  char *prev_token_state;
  bool streaming;
  bool blind_state;
  char **commands;
  int ccap;
  int csize;
  /* Requires next command for execution */
  bool next;
  /* Global file descriptor */
  int infile;
  command_t *lhs;
  command_t *rhs;
};

/* @state_machine: initialize new state machine */
state_machine_t *new_state_machine() {
  state_machine_t *sm = (state_machine_t *)malloc(sizeof(state_machine_t));
  sm->token_state = NULL;
  sm->prev_token_state = NULL;
  sm->blind_state = false;
  sm->streaming = false;
  sm->lhs = NULL;
  sm->rhs = NULL;
  sm->next = false;
  sm->ccap = DYNAMIC_SIZE;
  sm->csize = 0;
  /* Dynamic allocation commands container */
  sm->commands = (char **)malloc(CMD_DATA_SIZE * sm->ccap);
  return sm;
}

/* @state_machine: clear state */
void clear_state(state_machine_t *sm) { sm->token_state = NULL; }

/* @state_machine: add new command to command container */
void add_command(state_machine_t *sm, char *token) {
  assert(sm != NULL);
  int csize = sm->csize;
  /* Command container reaches its capacity */
  if (csize == sm->ccap) {
    sm->ccap = sm->ccap * 2;
    sm->commands = (char **)realloc(sm->commands, CMD_DATA_SIZE * sm->ccap);
  }
  /* Grow the container */
  int str_size = sizeof(char) * strlen(token) + 1;
  sm->commands[csize] = (char *)malloc(str_size);
  /* Add token into command container */
  strcpy(sm->commands[csize], token);
  sm->csize += 1;
}

/* Check if two strings are equal */
bool is_equal(char *strA, char *strB) { return strcmp(strA, strB) == 0; }

bool is_extra_command(char *token) {
  return is_equal(token, "cd") || is_equal(token, "source");
}

void extra_command(command_t *c) {
  if (is_equal(c->token, "source")) {
    process_script(c->args[1]);
    return;
  }
  if (is_equal(c->token, "cd")) {
    if (-1 == chdir(c->args[1])) {
      printf("No such file or directory: %s\n", c->args[1]);
    };
    return;
  }
}

void exec_cmd(command_t *c) {
  if (!is_equal(c->token, "") && execvp(c->token, c->args) == -1) {
    printf("%s: Command not found\n", c->token);
  }
}

/* @state_machine: Free allocated memory of commands */
void free_commands(state_machine_t *sm) {
  for (int i = 0; i < sm->csize; i++) {
    free(sm->commands[i]);
    sm->csize = 0;
  }
}

/* Sequence will be one sided command while other special commands will be two
 * sided */
bool is_two_sided_command(char *token) {
  return is_equal(token, IN_DIRECTION_CMD) ||
         is_equal(token, OUT_DIRECTION_CMD);
}

/* Check if the current execution must be output blind */
bool is_output_blind(char *next_token_state) {
  return is_two_sided_command(next_token_state) ||
         is_equal(next_token_state, PIPE_CMD);
}

/* Check if token is special (<>|;)*/
bool is_special_cmd(char *token) {
  return is_equal(token, SEQUENCE_CMD) || is_equal(token, PIPE_CMD) ||
         is_two_sided_command(token);
}

/* Blind state is for nest special commands like pipe or in / out direction
 * For example: a | b | c, we execute a | b first but we don't want it to print
 * 		the output to terminal so the state of @state_machine now set to
 * 		blind state and output is assigned to global state machine
 * 		file descriptor (read_fd / write_fd)
 * */
bool process_blind_state(state_machine_t *sm) {
  bool is_blind = is_output_blind(sm->token_state);
  if (is_two_sided_command(sm->token_state)) {
    sm->next = true;
  } else {
    sm->next = false;
  }
  return is_blind;
}

void reset_commands(state_machine_t *sm) {
  sm->lhs = NULL;
  sm->rhs = NULL;
}

void input_redirection_handler(state_machine_t *sm) {
  bool is_blind = process_blind_state(sm);
  command_t *lhs = sm->lhs;
  command_t *rhs = sm->rhs;
  char *filename = rhs->args[0];

  int p[2];
  assert(0 == pipe(p));
  /* Spawn a child to execute a command on left hand side*/
  pid_t pid = fork();
  if (pid == 0) {
    /* Close stdin and open new file descriptor at index 0 */
    assert(close(STDIN_FILENO) != -1);
    int fd = open(filename, O_RDONLY);
    assert(fd == STDIN_FILENO);

    /* Execute input redirection command */
    if (is_blind)
      dup2(p[1], STDOUT_FILENO);
    close(p[0]);
    exec_cmd(lhs);
    /* Blind output print to terminal */
    exit(1);
  } else {
    wait(NULL);
    close(p[1]);
    sm->infile = p[0];
    sm->blind_state = is_blind;
  }
  reset_commands(sm);
}

void output_redirection_handler(state_machine_t *sm) {
  bool is_blind = process_blind_state(sm);
  command_t *lhs = sm->lhs;
  command_t *rhs = sm->rhs;

  char *filename = rhs->args[0];
  /* Spawn a child to execute a command on left hand side*/
  pid_t pid = fork();
  if (pid == 0) {
    /* Close stdout and open new file descriptor at index 1 */
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);

    int fd2 = dup2(fd, STDOUT_FILENO);
    assert(fd2 == STDOUT_FILENO);
    assert(close(fd) != -1);

    /* Execute outpout redirection command */
    assert(dup2(sm->infile, STDIN_FILENO) == STDIN_FILENO);
    exec_cmd(lhs);
    exit(1);
  } else {
    wait(NULL);
    sm->blind_state = is_blind;
  }
  reset_commands(sm);
}

/* Single command handler */
void single_command_handler(state_machine_t *sm) {
  /* Spawn a child to execute command A */
  command_t *cmd = sm->lhs;

  if (is_extra_command(cmd->token))
    return extra_command(cmd);

  pid_t pid = fork();
  if (pid == 0) {
    assert(dup2(sm->infile, STDIN_FILENO) == STDIN_FILENO);
    exec_cmd(cmd);
    exit(1);
  } else {
    wait(NULL);
  }
  reset_commands(sm);
}

/* Sequence Handler (;)
 * Spawn a process to execute command A,
 * if there is no next token_state,
 * do the same thing for command B
 **/
void sequence_handler(state_machine_t *sm) { single_command_handler(sm); }

/* Pipe Handler (|)
 * Spawn a process A
 * Inside A, spawn process B
 * Close write_fd
 * Execute command LHS -> STDIN
 * Inside B, spawn process A
 * Close read_fd
 * Execute command RHS -> STDOUT
 * */
void pipe_handler(state_machine_t *sm) {
  bool is_blind = process_blind_state(sm);
  command_t *cmd = sm->lhs;

  if (is_extra_command(cmd->token))
    return extra_command(cmd);

  int p[2];
  assert(0 == pipe(p));

  pid_t pid = fork();
  if (pid == 0) {
    dup2(sm->infile, STDIN_FILENO);
    if (is_blind)
      dup2(p[1], STDOUT_FILENO);
    close(p[0]);
    exec_cmd(cmd);
    exit(1);
  } else {
    wait(NULL);
    close(p[1]);
    sm->infile = p[0];
  }
  sm->blind_state = false;
  reset_commands(sm);
}

void process_command(state_machine_t *sm, char *token, bool last) {
  if (is_special_cmd(token) || last) {
    /* Stage: Preprocessing */
    command_t *c = compact_command(sm->commands, sm->csize);
    char *state = sm->token_state;

    if (!last || sm->next) {
      sm->prev_token_state = state;
      sm->token_state = token;
    }

    /* If next is enabled, set RHS and execute */
    if (sm->next) {
      sm->rhs = c;
      state = sm->prev_token_state;
    } else {
      sm->lhs = c;
      state = sm->token_state;
    }

    if (last && !sm->next) {
      state = NULL;
    }

    /* Stage: Execution */
    if (state == NULL) {
      /* Single command handler */
      single_command_handler(sm);
    } else if (is_equal(state, IN_DIRECTION_CMD)) {
      /* Input Direction (<) */
      if (sm->next) {
        input_redirection_handler(sm);
      } else {
        sm->next = true;
      }
    } else if (is_equal(state, OUT_DIRECTION_CMD)) {
      /* Output Direction (>) */
      if (sm->next) {
        output_redirection_handler(sm);
      } else {
        sm->next = true;
      }
    } else if (is_equal(state, SEQUENCE_CMD)) {
      /* Sequence (;) */
      sequence_handler(sm);
    } else if (is_equal(state, PIPE_CMD)) {
      /* Pipe (|) */
      pipe_handler(sm);
    }
    /* Stage -> Clean up */
    free_commands(sm);
  } else {
    add_command(sm, token);
  }
}

/* Parse command and handle */
void parse_commands(char **commands, int size) {
  state_machine_t *sm = new_state_machine();
  sm->infile = 0; // read
  /* Start processing each token in commands list */
  for (int i = 0; i < size; i++) {
    process_command(sm, commands[i], i == size - 1);
  }
}

void print_help_page() {
  printf("Help\ncd > usage: cd <directory>\n\tChange the current working "
         "directory to the given one.\nsource > usage: source "
         "<script>\n\tExecute list of commands in the given text file "
         "script\nprev > usage: prev\n\tExecute the previous command\n");
}

void process_script(char *file) {
  FILE *fp;
  char *read = malloc(sizeof(char) * MAX_CMD_LEN + 1);
  char *buffer = malloc(sizeof(char) * MAX_CMD_LEN + 1);

  fp = fopen(file, "r");

  if (fp == NULL) {
    printf("Error: File %s not found\n", file);
    return;
  }

  while (read != NULL) {
    read = fgets(buffer, MAX_CMD_LEN, fp);

    vect_t *cmdVect = parse_token(buffer);
    /* convert cmdVect to list */
    char **cmdList = malloc((vect_size(cmdVect) + 1) * (sizeof(char *)));

    for (int i = 0; i <= vect_size(cmdVect); i++) {
      cmdList[i] = malloc(sizeof(char) * MAX_CMD_LEN + 1);
    }

    /* Add data to command list */
    for (int i = 0; i < vect_size(cmdVect); i++) {
      strcpy(cmdList[i], vect_get(cmdVect, i));
    }

    strcpy(cmdList[vect_size(cmdVect)], "\0");

    parse_commands(cmdList, vect_size(cmdVect) + 1);

    /* Freeing up command list memory allocated */
    for (int i = 0; i <= vect_size(cmdVect); i++) {
      free(cmdList[i]);
    }

    free(cmdList);
    vect_delete(cmdVect);
  }
  fclose(fp);
  free(read);
  free(buffer);
}

int start_shell() { // neeed to write call to parse script
  bool exit = false;

  char *line = malloc(sizeof(char) * MAX_CMD_LEN + 1);
  char *last = malloc(sizeof(char) * MAX_CMD_LEN + 1);
  printf(WELCOME_MSG);
  while (!exit) {

    char *buffer = malloc(sizeof(char) * MAX_CMD_LEN + 1);

    printf("%s", SHELL_PROMPT_MSG);

    line = fgets(buffer, MAX_CMD_LEN, stdin);
    if (line == NULL || is_equal(buffer, "exit\n")) {
      printf(EXIT_MSG);
      exit = true;
    } else {
      if (is_equal(buffer, "prev\n")) {
        strcpy(buffer, last);
        printf("> %s", buffer);
      } else if (is_equal(buffer, "help\n")) {
        print_help_page();
        continue;
      } else {
        strcpy(last, buffer);
      }
      vect_t *cmdVect = parse_token(buffer);

      /* convert cmdVect to list */
      char **cmdList = malloc((vect_size(cmdVect) + 1) * (sizeof(char *)));

      for (int i = 0; i <= vect_size(cmdVect); i++) {
        cmdList[i] = malloc(sizeof(char) * MAX_CMD_LEN + 1);
      }

      /* Add data to command list */
      for (int i = 0; i < vect_size(cmdVect); i++) {
        if (i < vect_size(cmdVect)) {
          strcpy(cmdList[i], vect_get(cmdVect, i));
        }
      }

      strcpy(cmdList[vect_size(cmdVect)], "\0");

      /* Starting parsing command */
      parse_commands(cmdList, vect_size(cmdVect) + 1);

      /* Freeing up command list memory allocated */
      for (int i = 0; i <= vect_size(cmdVect); i++) {
        free(cmdList[i]);
      }

      free(buffer);
      free(cmdList);
      vect_delete(cmdVect);
    }
  }
  free(last);
  free(line);
  return 0;
}

/* Shell program entry point */
int main(int argc, char **argv) { return start_shell(); }
