**Intended for a Linux system**
# Simple Shell Interpreter with Background Process Management

This project is a simple custom shell implementation that allows basic shell commands, background process management, and navigation commands such as `cd`. It uses the GNU Readline library to handle command-line input and can manage background processes.

## Features

- **Command Prompt**: Dynamically displays the current user, hostname, and working directory.
- **Foreground Commands**: Executes shell commands entered by the user.
- **Background Commands (`bg`)**: Run commands in the background and track their status.
- **Background Process Listing (`bglist`)**: Lists all currently running background processes.
- **Directory Navigation (`cd`)**: Allows the user to change the current working directory.
- **Exit (`exit`)**: Terminates the shell.

## Usage

1. Compile the code using a C compiler, linking it with the GNU Readline library.

2. Run the shell executable.

### Available Commands

- **Foreground Commands**: Enter any valid shell command, and it will be executed in the foreground.
  
- **Background Command (`bg`)**: Run commands in the background using the `bg` command followed by the command to execute.
  Example:
  ```bash
   bg sleep 10
  ```

- **List Background Processes (`bglist`)**: Displays all the currently running background processes.

- **Change Directory (`cd`)**: Change the current working directory by providing a valid path.
  Example:
  ```bash
   cd /path/to/directory
  ```

- **Exit (`exit`)**: Close the shell and terminate the program.

## Background Process Management

- **Adding to Background List**: The shell tracks background processes using a linked list. When a background command is executed, the process ID (PID) and command string are stored in the list.
  
- **Checking Background Processes**: The shell regularly checks for terminated background processes using a non-blocking `waitpid()` call.

## Code Structure

- **Linked List**: Background processes are stored in a linked list (`struct bg_pro`).
  
- **Process Management**: Background processes are created using the `fork()` system call. Commands are executed using `execvp()`.

- **Command Parsing**: User input is parsed and split into command tokens.

- **Readline Library**: The GNU Readline library is used to handle user input efficiently.

## Dependencies

- **GNU Readline**: Ensure the GNU Readline library is installed on your system.
  ```bash
   sudo apt-get install libreadline-dev
  ```

## Future Improvements

- Add signal handling to manage background processes more robustly and handle process termination.
- Support for piping and redirection.
- Improve error handling for invalid commands.

## License

This project is open source and available under the MIT License.
