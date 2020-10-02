#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <grp.h>
#include<fcntl.h> 
#include <unistd.h>

#include "tokenizer.h"

/* Macro for using group as struct */
#define group struct group

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_id(struct tokens *tokens);

/* Built-in command functions take token array and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc
{
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_id, "id", "display the user-id, the primary group-id and the groups the user is part of"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens)
{
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens)
{
  exit(0);
}

/* id command */
int cmd_id(unused struct tokens *tokens)
{
  char userName[2048];
  group *Group = getgrgid(getgid());
  int ngroups = 0;
  getlogin_r(userName, 2048);

  getgrouplist(userName, Group->gr_gid, NULL, &ngroups);
  __gid_t groups[ngroups];
  getgrouplist(userName, Group->gr_gid, groups, &ngroups);

  printf("uid=%d(%s) gid=%d(%s)\n",
         (int)getuid(),
         userName,
         Group->gr_gid,
         Group->gr_name);
  printf("groups=");
  for (int i = 0; i < ngroups; i++)
  {
    group *gr = getgrgid(groups[i]);
    if (gr == NULL)
    {
      perror("Fatal error: Group not found!");
      return -1;
    }
    printf("%d(%s) ", gr->gr_gid, gr->gr_name);
  }
  printf("\n");
  return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[])
{
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell()
{
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive)
  {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[])
{
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "Dipanshu's Shell %d > ", line_num);


  while (fgets(line, 4096, stdin))
  {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0)
    {
      cmd_table[fundex].fun(tokens);
    }
    else
    {
      /* REPLACE this to run commands as programs. */
      char *runFile = (char *)malloc(sizeof(char)*4096);
      char *outPutFile = (char *)malloc(sizeof(char)*4096);
      bool redirection=false;
      int output=1;
      int i=0;
      for(;i<4096;i++){
        if(line[i]==10 || line[i]==' ' || line[i]=='>'){
          runFile[i]='\0';
          break;
        }
        runFile[i]=line[i];
      }
      for(;i<4096;i++){
        if(line[i]=='>'){
          redirection=1;
          i++;
          break;
        }
      }
      if(redirection){
        // stripping initial whitespaces
        while(line[i]==' ')i++;
        int startOutputFileIndex=i;
        for(;i<4096;i++){
          if(line[i]==10 || line[i]==' '){
            outPutFile[i-startOutputFileIndex]='\0';
            break;
          }
          outPutFile[i-startOutputFileIndex]=line[i];
        }
      }
      if(redirection && strlen(outPutFile)){
        output = open(outPutFile,O_WRONLY | O_CREAT | O_TRUNC);
      }
      else if(redirection && !strlen(outPutFile)){
        fprintf(stdout, "Redirection file not defined\n");
      }
      int pid = fork();
      char *args[] = {runFile, NULL};
      if (pid == 0){
        // child process
        int old_stdout = dup(1);
        if(output!=1) dup2(output, 1);
        execv(args[0], args);
        if(output!=1) close(output);
        dup2(old_stdout,1);
        fprintf(stdout, "%s: Command not found. Use ? for help.\nNOTE To run a executable in same directory make sure you use './<executable>'\n", args[0]);
        return 0;
      }
      // parent will wait for the child process to complete
      else wait(NULL);

    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "Dipanshu's Shell %d > ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
