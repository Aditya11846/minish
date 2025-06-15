Project: `minish` – A Minimal Shell in C

## Overview
You're building a minimal shell that mimics basic UNIX shell functionality. It should be able to:
- Run external commands (`ls`, `cat`, etc.)
- Handle built-in commands like `cd` and `exit`
- Support I/O redirection (`>`, `>>`, `<`)
- Pipe commands (`ls | grep .c`)
- Run commands in the background (`&`)

## Shell Feature Details

### Run External Commands
- Use `fork()` to create a child process.
- In the child, use `execvp()` to run commands like `ls`, `echo`, etc.
- The parent waits using `waitpid()` unless the command is running in the background.

### Built-in Commands (`cd`, `exit`)
- These must be handled in the parent process since they affect shell state.
- `cd <dir>`: use `chdir()` to change working directory.
- `exit`: use `exit(0)` to terminate the shell.

### I/O Redirection
- Redirect standard input or output using `dup2()` after opening files with `open()`.
- Example: `cat < input.txt`, `grep foo > output.txt`, or `>>` for append mode.

### Piping Commands
- Use `pipe()` to create a pipe.
- Fork twice to create two children. Connect stdout of the first to the write end and stdin of the second to the read end using `dup2()`.
- Chain more than two commands with multiple pipes.

### Background Execution (`&`)
- If a command ends with `&`, the shell should not wait for the child process.
- Detect this token in input and skip the `waitpid()` step.

### Signal Handling
- Use `signal(SIGINT, handler_function)` to prevent shell from terminating on Ctrl+C.
- Only the foreground process should be killed by signals like SIGINT.
- Recommended: Reset signal handling to default in child processes.

## Getting Started
### Step 1: Read User Input
Use `fgets()` to read a line from `stdin`. Strip the newline character.

### Step 2: Tokenize Input
Use `strtok()` or a manual tokenizer to split the input into tokens (command + args).

### Step 3: Handle Built-in Commands
Check if the command is `cd`, `exit`, or another built-in and handle it manually in the parent process.

### Step 4: Fork and Execute External Commands
```c
pid_t pid = fork();
if (pid == 0) {
    // child process: run execvp(argv[0], argv);
} else {
    // parent process: wait for child (unless background)
}
```

### Step 5: Handle Redirection and Pipes
- Use `dup2()` to redirect `stdin`/`stdout` before calling `execvp()`
- Use `pipe()` and `fork()` to build a pipeline for `cmd1 | cmd2`

### Step 6: Handle Signals
Catch `SIGINT` (Ctrl+C) so the shell doesn’t exit on user interrupt.

## File Structure
- `minish.c`: main shell implementation
- `Makefile`: to build the shell

## Build & Run
```bash
make
./minish
```

## Clean
```bash
make clean
```

## Try These
```sh
ls -l
cd ..
cat < input.txt | grep hello > output.txt &
```

## Useful Functions
- `fgets`, `strtok`, `strcmp`
- `fork`, `execvp`, `waitpid`, `dup2`, `pipe`
- `open`, `close`, `chdir`, `signal`

## Bonus Features
- Command history
- Tab completion (readline)
- Custom prompt
