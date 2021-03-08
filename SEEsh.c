#include<stdio.h>     // printf()
#include<stdlib.h>    // malloc(), free(), SEEsh_exit(), execvp()
#include<string.h>    // strcmp(), strtok()
#include <unistd.h>   // fork(), chdir(), exec(), pid_t()
#include <sys/wait.h> // waitpid()
#include <signal.h>
#define  _POSIX_C_SOURCE 200809L

void SEEsh_loop(void);
char *read_line(void);
char **tokenize_line(char *line);
int SEEsh_launch(char **args);
int cd(char **args);
int help(char **args);
int SEEsh_exit(char **args);
int SEEsh_num_builtins(void);
int SEEsh_execute(char **args);
int pwd(char **args);
int set(char **args);
int unset(char **args);
extern char **environ;
int (*builtin_func[]) (char **);
char *builtin_commands[];

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num){
    signal(SIGINT, sigintHandler);
    printf("\nPlease use Ctrl+D or exit to terminate\n");
    printf("? ");
    fflush(stdout);
}

int main(int argc, char *argv[]){
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  char **tokens;
  signal(SIGINT, sigintHandler);
  fp = fopen(".SEEshrc", "r");
   if (fp == NULL){
     printf(".SEEshrc could not be opened");
     exit(EXIT_FAILURE);
   }
   while (getline(&line, &len, fp) != -1) {
     tokens = tokenize_line(line);
     SEEsh_execute(tokens);
   }
  fclose(fp);
  if(line){
    free(line);
  }
  free(tokens);
  SEEsh_loop();
  return(EXIT_SUCCESS);
}

//command loop : read, parse, excecute
void SEEsh_loop(void){
  //will hold line read from stdin
  char *line;
  //will hold tokens from line
  char **commands;
  int status = 1;
  do {
    printf("? ");
    //read a line
    line = read_line();
    //tokenize line
    commands = tokenize_line(line);
    status = SEEsh_execute(commands);
    free(commands);
    free(line);
    fflush(stdout);
  }while(status);
}

int SEEsh_execute(char **args){
  //Control-D was entered, SEEsh_exit program
  if(args[0] ==NULL){
    return 0;
  }
  if (args[0] == NULL){
    //empty command was enter
    printf("Please enter a command");
    return 1;
  }
  int i;
  for ( i = 0 ; i < SEEsh_num_builtins(); i++){
    //iterates through list of built in commands
    if(strcmp(args[0], builtin_commands[i]) == 0){
      //check to see what command is entered first
      return (*builtin_func[i])(args);
    }
  }
  //function was not a builtin
  return SEEsh_launch(args);
}

char *builtin_commands[] = {"cd", "help", "exit", "pwd", "set", "unset"};


//array of function pointers for built in commands
int (*builtin_func[]) (char **) = {
  &cd,
  &help,
  &SEEsh_exit,
  &pwd,
  &set,
  &unset
};

//returns the number of built in commands
int SEEsh_num_builtins(void) {
  return sizeof(builtin_commands) / sizeof(char *);
}

//returns the number of environment variables
int SEEsh_num_vars(void){
  return sizeof(environ)/ sizeof(char*);
}

//changes working directory
int cd(char **args){

  if(args[1] ==NULL){
    //make sure user specified directory to change to
    fprintf(stderr, "SEEsh: cd: expected argument after \"cd\"\n");
  }
  else{
    //chdir changes the current working directory to args[1]. returns 0 upon sucsess
    if (chdir(args[1]) != 0){
      fprintf(stderr, "SEEsh: cd: current working directory could not be changed to \"%s\"\n", args[1]);
    }
  }
  return 1;
}

//prints out list of built in commands
int help(char **args){
  int i;
  printf("\n");
  printf("->Sonja Skagestad's SEEsh\n");
  printf("->Enter the command name and arguments, then hit enter.\n");
  printf("->The following commands are built in:\n");
  for(i = 0 ; i < SEEsh_num_builtins() ; i++){
    printf("->%s\n", builtin_commands[i]);
  }
  return 1;
}

//SEEsh_exit shell
int SEEsh_exit(char **args){
  return 0;
}

//print full path of working directory
int pwd(char **args){
  char cwd[512];
  getcwd(cwd, sizeof(cwd));
  printf("Current working dir: %s\n", cwd);
  return 1;
}

//set environment variables
int set(char **args){
  //print out all environment variables and values
  if(args[1] == NULL){
    for(char **env = environ ; *env ; ++env){
      printf("environment variable: %s\n", *env);
    }
  }
  else{
    char * token  = args[1];
    //set variable to empty string
    if(args[2] == NULL){
      char * val = "";
      if(setenv(token,val, 1)!=0){
        printf("could not set %s\n", token);
      }
    }
    else{
      //set environment variable to val
      if(setenv(token,args[2], 1)!=0){
        printf("could not set %s to %s\n", token, args[2]);
      }
    }
  }
  return 1;
}

//unsets the specified environment variable
int unset(char **args){
  if(args[1] == NULL){
    printf("please enter a variable to unset.\n");
  }
  else{
    if(unsetenv(args[1]) != 0){
      printf("Unable to unset %s.", args[1]);
    }
  }
  return 1;
}

//launches new child process
int SEEsh_launch(char **args){
  pid_t processID;
  int status;
  //duplicate parent process. Now have two processes running concurrently
  //fork returns 0 to the child process, which will excecute first
  processID = fork();
  if(processID == 0){
    //child process
    if(execvp(args[0], args) == -1){
      //if execvp returns there was an error
      fprintf(stderr, "SEEsh: lsh_launch: Error occurred during execvp()\n");
      fprintf(stderr, "Please enter a command. Press help for more info\n");
    }
    exit(EXIT_FAILURE);
  }
  else if( processID < 0){
    //error occurred during fork()
    fprintf(stderr, "SEEsh: lsh_launch: Error occurred during fork()\n");
  }
  else{
    //parent process
    do{
      waitpid(processID, &status, WUNTRACED);
      //request information from excecuting child process
    }
    while(!WIFEXITED(status) && !WIFSIGNALED(status));
    //continue checking on child process for completion
  }
  return 1;
  //indicates the process was SEEsh_exited or killed, prompt for input again
}

#define BUFSIZE 512
char *read_line(void){
  int bufsize = BUFSIZE;
  int index = 0;
  int c;
  char *buffer = malloc((sizeof(char)) * bufsize);
  if (!buffer){
    fprintf(stderr, "SEEsh: read_line: allocation error\n" );
    exit(EXIT_FAILURE);
  }
  while (1) {
    // Read a character
    c = getchar();
    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[index] = '\0';
      return buffer;
    } else {
      buffer[index] = c;
    }
    index++;
    // If we have exceeded the buffer, print error message.
    if (index >= bufsize) {
      fprintf(stderr,"SEEsh: read_line: input exceeded 512 characters\n");
      exit(EXIT_FAILURE);
    }
  }
}

#define TOKEN_DELIM " \t\n="
char **tokenize_line(char *line){
  int bufsize = BUFSIZE;
  int index = 0;
  //allocate memory for an array of pointers that will contain the tokens
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;
  if(!tokens){
    fprintf(stderr, "SEEsh: tokenize_line: memory allocation error\n");
    exit(EXIT_FAILURE);
  }
  //read in a token from line
  token = strtok(line,TOKEN_DELIM);
  while(token != NULL){
    tokens[index] = token;
    index++;
    //exceeded 512 characters
    if(index >= BUFSIZE){
      fprintf(stderr, "SEEsh: tokenize_line: input exceeded 512 characters\n");
      exit(EXIT_FAILURE);
    }
    //read in next token
    token = strtok(NULL, TOKEN_DELIM);
  }
  //add NULL terminator in last position of char array
  tokens[index] = NULL;
  return tokens;
}
