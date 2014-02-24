// KNOWN BUGS: 
// file redirection in a directory that is not write-accessible causes failing errors


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#define BUFFERLENGTH  512


// RETURNS A SAVED COPY OF STDOUT, AND REPLACED STDOUT WITH FILE
int closeSTDOUT(char *file){
  int save = dup(STDOUT_FILENO);
  int rc = close(STDOUT_FILENO);
  if (rc < 0){
    perror("close_closeSTDOUT");
    return -1;
  }
  int fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
  //printf("opened file %s with file number %d, which should be %d", file, fd, STDOUT_FILENO);
  if (fd < 0){
    perror("open_closeSTDOUT");
    return -1;
  }  
  return save;
}


// CLOSES THE FILE IN STDOUT'S PLACE AND RESTORES STDOUT
int openSTDOUT(int save){
  int rc = close(STDOUT_FILENO); //actually is the file in it's place
  if (rc < 0){
    perror("close_openSTDOUT1");
    return -1;
  }
  int fd = dup2(save, STDOUT_FILENO);
  if (fd < 0){
    perror("open_STDOUT");
    return -1;
  }
  rc = close(save);
  if (rc < 0){
    perror("close_closeSTDOUT2");
    return -1;
  }
  return fd;
}


// MAIN FUNCTION
int main (int argc, char* argv[]){

  // VARIABLES 
  char buff[BUFFERLENGTH];
  const char del2[] = " \t\n\v\f\r";
  const char del1[] = ">";
  char *token[BUFFERLENGTH]; // TODO: dyanmiically allocate the array of char*
  char *redirect_file;
  char *redirect_command;
  char *pypart = ".py";

  int count, i, carrotcount, andcount;
  char error_message[30] = "An error has occurred\n";
  //write(STDERR_FILENO, error_message, strlen(error_message));

  int rc, fd;
  int saved_STDOUT = -1;
  int numprocess;
  FILE *file;
  

// Batch mode arg
  if (argc == 2){
    file = fopen(argv[1],"r");
    if (file == NULL){
      write(STDERR_FILENO, error_message, strlen(error_message));
      //perror("cannot open batch file");
      exit(1);
    }
  }
  //If invalid args for mysh exit
  if (argc > 2) {
     write(STDERR_FILENO, error_message, strlen(error_message));
     exit(1);
  }

  // MAIN LOOP, PROMPTS USER AND PROCESSES INPUT
  while(1){
    // INITIALIZE COUNTS
    carrotcount = 0;
    andcount = 0;
    numprocess = 0;

    // PROMPT USER IF IN INTERACTIVE MODE
    if (argc != 2){
      printf("mysh> ");
    }
  
    // READ INPUT UNTIL NEWLINE READ
    // normal mode
    if (argc != 2){
      if (fgets(buff, BUFFERLENGTH, stdin) == NULL){
        //perror("fgets");
        write(STDERR_FILENO, error_message, strlen(error_message));
        continue;
      } 
      //batch mode
    } else if (argc == 2){
      if (fgets(buff, BUFFERLENGTH, file) == NULL){
        //Done with batch file
        exit(0);
      }
      // PRINT LINE
      write(STDOUT_FILENO,buff,strlen(buff));
    }

    // COUNT ">" and "&"
    for (i = 0; i < strlen(buff); i++){
      if (buff[i] == '>'){
 	      carrotcount++;
      }
      else if (buff[i] == '&'){
        andcount++;
        buff[i] = ' ';
      }
    }

    // COUNT FILE REDIRECTION CHARACTERS
    if (carrotcount > 1){
      perror("too many >");
      continue;
    }

    // OUTPUT DIRECTION DETECTED!
    if (carrotcount == 1){
      redirect_command = strtok(buff, del1);
      //printf("Command buffer contains:%s\n", redirect_command);
      //char temp[255];
      //sprintf(temp,"Command buffer contains:%s\n",redirect_command);
      //write(STDOUT_FILENO,temp,strlen(temp));
      
      redirect_file = strtok(NULL, del1);
      
      // PARSE REDIRECTED FILE FROM PORTION AFTER THE ">"
      redirect_file = strtok(redirect_file, del2);
      if (strtok(NULL, del2) != NULL){
        // CHANGE ERROR
//	perror("too many output files for redirection");
   write(STDERR_FILENO, error_message, strlen(error_message));
	continue;
      }
      //printf("Output file buffer contains:%s\n", redirect_file);

      // SAVE STDOUT AND OPEN THE NEW FILE FOR REDIRECTION
      saved_STDOUT = closeSTDOUT(redirect_file);
      if (saved_STDOUT < 0){
        // CHANGE ERROR
        perror("saving STDOUT");
        continue;
      }      
    }

    // PARSE INPUT AMD REMOVE WHITESPACES
    count = 0;
    token[0] = strtok(buff, del2);
    while (token[count] != NULL){
      count++;
      token[count] = strtok(NULL, del2);
    }

    // NEW PARSE CODE
//    count = 1;
//    token[0] = strtok(buff, del2);
//    while 

    // DEBUG: PRINT TOKENS
    /*
    write(STDOUT_FILENO, "TOKENS:", 7);
    for (i = 0; i < count; i++){
      //printf("token[%d]: %s\t",i,token[i]);
      write(STDOUT_FILENO, "\n", 1);	
      write(STDOUT_FILENO, token[i], strlen(token[i]));
      write(STDOUT_FILENO, "END", 3);
    }
    write(STDOUT_FILENO, "\n", 1);
      */

    // EXIT - COMPLETE
    if (strncmp(token[0], "exit", 512) == 0){
      exit(0);
    }

    // CD - COMPLETE
    if (strncmp(token[0], "cd", 512) == 0){
      char *home;
      
      if (token[1] == NULL){
	home = getenv("HOME");
	if (chdir(home) != 0){
          // CHANGE ERROR
          write(STDERR_FILENO, error_message, strlen(error_message));
          //perror("error in CD, no arguments");
        }
      }
      else{
	if (chdir(token[1]) != 0){
	  // CHANGE ERROR
          write(STDERR_FILENO, error_message, strlen(error_message));
          //perror("error in CD, with arugments");
        }
      }
      continue;
    }    

    // PWD - COMPLETE
    if (strncmp(token[0], "pwd", 512) == 0){
      char cwd[512];
      if (getcwd(cwd, 512) == NULL){
        // CHANGE ERROR
        write(STDERR_FILENO, error_message, strlen(error_message));
        //perror("error in PWD");
      }
      write(STDOUT_FILENO, cwd, strlen(cwd));
      write(STDOUT_FILENO, "\n", 1);
      continue;
    }

    // WAIT - THINK ITS DONE
    int pid;
    if (strncmp(token[0], "wait", 512) == 0){
      while(numprocess > 0){
        pid = wait(NULL);
        if (pid != -1){
          numprocess--;
         }
      }
      //if (strncmp(token[1],"NULL", 512) != 0)
      //   write(STDERR_FILENO, error_message, strlen(error_message));
      continue;
    }

    // PYTHON INTERPRETER - IN PROGRESS
    if (strstr(token[0], pypart) != NULL){
     printf("Python file (%s) found. Count = %d", token[0], count);
     token[count+1] = NULL;
     for (i = count; i > 0; i--){ // SHIFT EVERYTHING TO THE RIGHT
       token[i] = token[i-1];
     }
      token[0] = "python";
    }
  
    // OTHER COMMANDS
    rc = fork();
    if (rc < 0){
      fprintf(stderr, "fork() failed\n");
    } else if (rc == 0) {
      execvp(token[0], token);      
    } else if (andcount == 0){
      int wc = wait(NULL);
      continue;
    }

    if (saved_STDOUT != -1){
      fd = openSTDOUT(saved_STDOUT); // RESTORE STDOUT
      if (fd < 0){
        //perror("opening STDOUT");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); // TODO: HANDLE THIS MORE GRACEFULLY
      }
      saved_STDOUT = -1;
    }

  } // End of main loop

  return 0; // WE'LL NEVER GET HERE!
}
