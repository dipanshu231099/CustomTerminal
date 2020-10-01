## Custom Terminal

""" The provided shell code follows the same dispatch pattern used in operating systems (https://0xax.gitbooks.io/linux-insides/SysCall/linux-syscall-2.html).In the OS, the system calls are implemented as software interrupts. Whenever a system call is invoked, an exception is generated and the appropriate handler from the system call table is invoked. Here we look up commands entered by the user in a cmd table and transfer control to the appropriate command handler. So far, the only commands supported are ? for help menu, and exit to quit the shell. """

- the shell had "?" and "exit" functionalities already provided as in the link above.

### Functionalities in the terminal:
- ? (display this help related to various functions)
- exit (to exit the shell)
- id (to display the user-id, the primary group-id and the groups the user is part of)

### Compiling the shell
- ```make```
- ```./shell```
