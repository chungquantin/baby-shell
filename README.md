# Basic Shell

<img width="100%" alt="Screen Shot 2023-03-06 at 10 03 25" src="https://user-images.githubusercontent.com/56880684/223148385-1b600f1f-3848-43b1-ab3d-e9fa38478dba.png">

## Description

Reimplement the OS Shell in C using libc API. This shell covers most of the feature of a shell.

## How does shell works?

Shell is simply an interface layer that takes the advantage of `libc` API to operate a specific command. Terminal and shell is a two different concepts. Terminal is an application that uses shell as backend for user to communicate with the kernel.

### Tokenizer

First stage of a shell is to tokenize the command into tokens that will be processed in a later stage for command execution. You can consider a tokenizer is a preprocessing stage before moving it to the main cook. There is a rule for tokenizing:

- Special tokens: `,`, `;`, `|`
- String: `"Hello world"`

### Command execution

To understand how shell execute commands, you must have a fundamental knowledge of process in operating system. Whenever a process uses system call like `execve` or `execvp` to execute binary command, the process exits with success code `exit(0)` or error code `exit(1)`. Hence, single process shell unable to run interactively waiting for user input.

#### Forking process

The solution is simple, for every command, we will fork another process which copies the memory of that process to the another. In this way, the context of the old process is the same while the main program is no longer interrupts after the execution.

```
NAME
     fork -- create a new process

SYNOPSIS
     #include <unistd.h>

     pid_t
     fork(void);

DESCRIPTION
     fork() causes creation of a new process...
```

Read from `man fork`

### Pipe

Pipelining command is a special thing in every shell, it makes use of OS file descriptors to stream the output from `stdout` to a file descriptor or to read the input from file descriptor instead of `stdin`. Implementing a pipe requires an API called `pipe()`.

```
NAME
       pipe - Postfix delivery to external command

SYNOPSIS
       pipe [generic Postfix daemon options] command_attributes...

DESCRIPTION
       The pipe(8) daemon processes requests from the Postfix queue ma
nager to...
```

Read from `man pipe`

## Features

- [x] Execute single command
- [x] Sequencing and execute multiple commands (Token: `;`)
- [x] Input / Output redirection (Token: `>`, `<`)
- [x] Pipelining command (Token: `|`)
- [x] Executing scripts (Token: `source`)
- [ ] Auto code completion
- [ ] SSH remote connection
- [ ] Custom config file similar to `.zshrc` or `.bashrc`

## Development Guideline

The [Makefile](Makefile) contains the following targets:

- `make all` - compile everything
- `make tokenize` - compile the tokenizer demo
- `make tokenize-tests` - compile the tokenizer demo
- `make shell` - compile the shell
- `make shell-tests` - run a few tests against the shell
- `make test` - compile and run all the tests
- `make clean` - perform a minimal clean-up of the source tree

The [examples](examples/) directory contains an example tokenizer. It might help.

## Contributions

Created by **Micheal Baraty** and **Tin Chung**
