/******************************************************************
 * Program 3 smallsh
 * Author: Zack Chand
 * Description: Creating a small shell using the C language
 * Date 02 May 2022
 ******************************************************************/

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
/****************************
 *  GLOBAL VARIABLES
 *****************************/
struct sigaction sigINT = {0}; // For CTRL + C
struct sigaction sigSTP = {0}; // For CTRL + Z

#define MAX_LENGTH 2048
#define EXIT_ONE 1
#define EXIT_TWO 2
int exitChild = 1;
int flag = 1;
int foreGround;
bool backgroundProcess = false; // Variable that determines if the process
bool executeFork = true;
bool inputFileFlag = false;
bool outputFileFlag = false;
char inputFileName[50];
char outputFileName[50];
char *inputBuffer[512];


/***************************************************************
*  Function: printTermstatus
*  Description: prints the termination signal of the child processs
*  Recieves:int status 
*  Outputs: Custom message with int value 
***************************************************************/

void printTermStatus(int status) {
  if (WTERMSIG(status)) {
    int term_sig = WTERMSIG(status);
    printf("terminated by signal %d\n", term_sig);
  }
}
/***************************************************************
*  Function: childStatus
*  Description: prints the query status if the child process ended normally
*  Recieves: int status
*  Outputs: Custom message
***************************************************************/

void childStatus(int status) {
  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    printf("Exit status of the child was %d\n", exit_status);
  }
}
/***************************************************************
*  Function:Allowbackground 
*  Description: if the fore ground and background process is true deterimined from the input the return true
*  Otherwise return false 
*  Recieves: None it looks at the global variables for the values
*  Outputs:  None just ethier returns true or false
***************************************************************/

bool allowBackground() {
  if (foreGround == 1 && backgroundProcess == true) {
    return true;
  }

  else {

    return false;
  }
}
/***************************************************************
 *  Function: sigSTPHandler
 *  Description: Function that handles the CTRL + Z and ignores keypress and
 *changes foreground to determine if it is a background process Recieves: int
 *signo Outputs: Custom message
 ***************************************************************/

void sigSTPHandler(int signo) {
  char *writeMessage;
  if (foreGround == 1) {
    writeMessage = "Entering foreground-only mode (& is now ignored)\n";
    fflush(stdout);
    write(1, writeMessage, 49);
    foreGround = 0;
  }
  if (foreGround == 0) {
    writeMessage = "Exiting foreground-only mode\n";
    write(1, writeMessage, 29);
    foreGround = 1;
    fflush(stdout);
  }
}
/***************************************************************
 *  Function: changeDirectory()
 *  Description: changes current working directory and if no directory
 *  is specified then return to the "home" directory
 *  Recieves: char *
 *  Outputs: None or error if no directory found
 *  References exploration directories
 ***************************************************************/
void changeDirectory(char *directoryInput[]) {
  if (directoryInput[1]) { //If there is a specified directory then change to it 
    if (chdir(directoryInput[1]) == -1) { //
      printf("Directory not found.\n");
      fflush(stdout);
    }
  } else { //Otherwise change to the home directoey
    // If directory is not specified, go to ~
    chdir(getenv("HOME"));
  }
}

/****************************************************************
 *  Function : getUserInput()
 *  Descrption : get users input and parses through the input to to determine
 *  what to do next
 *. Recieves : Char arrays and int values outputs : int number.
 *  If user types "exit" it will tell main to break out of loop to stop program
 * Referenced geeks for geeks on how to parse through elements
 ***************************************************************/
int getuserInput(char *buffer[], int pid) {
  char inputArray[512];
  int i = 0;
    
  printf(": ");
  if (fgets(inputArray, sizeof(inputArray), stdin))
    ;

  inputArray[strcspn(inputArray, "\n")] = 0; // Gets rid of the newline

  // If no input then return to prevent seg fault
  if (!strcmp(inputArray, "")) {
    buffer[0] = strdup("");
    return 1;
  }
  char *value = strtok(inputArray, " "); // Get the first string from the input
  // Use strncmp to prevent overrun that can cause seg fault
  while (value != NULL) { 
    
    // Using strtok parse through input
    if (!strncmp(value, "&", 1)) { // Trigger background process if & in input
      backgroundProcess = true;
    }

    else if (!strncmp(value, "<", 1)) {
      value = strtok(NULL, " "); // Gets the next string for input file name
      strcpy(inputFileName, value); //Copy string to global variable to be used for fork
      inputFileFlag = true;
    }

    else if (!strncmp(value, ">", 1)) {
      value = strtok(NULL, " "); // Gets the next string for output file name
      strcpy(outputFileName, value);   //Copy string to global variable to be used for fork
      outputFileFlag = true;

    } else {
      buffer[i] = strdup(value);   //Otherwise copy to helper array that will help with inputs
      
      }

      value = strtok(NULL, " ");
      i++;
    }
    return 0;
  }
  /***************************************************************
   *  Function:getCMD()
   *  Description: gets and parses through the raw input to determine
   *  if input, output file commands are present or & for background procceses
   *  Recieves: raw input array
   *  Outputs: Error messages and echo messages
   ***************************************************************/
  int getCMD(char *inputArray[]) {
    char path[200];

    int i = 0;
    if (strcmp(inputArray[0], "#") == 0) {
      return 0;
    }
    if (strcmp(inputArray[0], "cd") == 0) {

      changeDirectory(inputArray);

    } else if (strcmp(inputArray[0], "status") == 0) {
      childStatus(exitChild);
      return 0;
    }

    else if (strcmp(inputArray[0], "exit") == 0) {
      flag = 0;
      executeFork = false;
      return 0;
    }
    return 1;
  }
  /***************************************************************
   *  Function: openFiles()
   *  Description: Depending on what the user entered in the function will run
   * if students to see if ethier input file or output file is detected
   *  Recieves: char [] ,char[] , bool , bool
   *  Outputs: Error messages if an error has occured
   ***************************************************************/
  void openFiles(char inputFileName[], char outputFileName[],
                 bool inputFileFlag, bool outputFileFlag) {

    int inputFile;
    int outputFile;
    int result;

     //FROM EXPLORATION:FILES in module 3
    if (inputFileFlag == true) {

      inputFile = open(inputFileName, O_RDONLY); //Only open and read the file only
      if (inputFile < 0) {
        perror("Failed to open file\n");
        exit(EXIT_ONE);
      }
      result = dup2(inputFile, 0);
      if (result < 0) {
        perror("Error assign failed\n");
        exit(EXIT_TWO);
      }
    }
    if (outputFileFlag == true) {

      //FROM EXPLORATION:FILES in module 3
      outputFile = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC , 0600);
      if (outputFile < 0) {
        perror("Cannot open file\n");
        exit(EXIT_ONE);
      }
      result = dup2(outputFile, 1);      //REFERENCE https://man7.org/linux/man-pages/man2/dup.2.html on how dup2 is used
      if (result < 0) {
        perror("Error assign failed\n");
        exit(EXIT_TWO);
      }
    }
  }
  /***************************************************************
   *  Function: forkProccess
   *  Description: Forks procceses and executes commands in bash
   *  Recieves: char *, struct
   *  Outputs: Error messages if it is unable to fork
  `*  This code can be refered to https://brennan.io/2015/01/16/write-a-shell-in-c/ h
   *  that was made in the annocement
   ***************************************************************/

  void forkProcess(char *command[], struct sigaction sigInt) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
      // Child process

      openFiles(inputFileName, outputFileName, inputFileFlag, outputFileFlag); //opened filed 
      
      if (execvp(command[0], command) == -1) {
        perror("no such file or directory\n");
        exit(1);
      }

    } else if (pid < 0) {
      // Error forking
      perror("ERROR FORKING");
      exit(1);
    } else {
      // Parent process
      do {

        if (allowBackground()) {    //Execute process in the background
          wpid = waitpid(pid, &exitChild, WNOHANG);
          printf("background pid is %d\n", pid);
          fflush(stdout);
        } else {                  //therwise execute it normally
          wpid = waitpid(pid, &exitChild, 0);
        }

      } while (!WIFEXITED(exitChild) && !WIFSIGNALED(exitChild));
      // printf("child  %d terminated\n", pid);  //For some reason these two lines of codes cause the testcript to fail
      // printTermStatus(exitChild);
    }
  }
  /***************************************************************
   *  Function:ignoreSIGINT
   *  Description: Catches the SIGINT signal and ignores it so the program
   *cannot be termiated Recieves: struct Outputs:  None
   ***************************************************************/
  void ignoreSIGINT(struct sigaction sigINT) {
    sigINT.sa_handler =
        SIG_IGN; // Catches the signal from SIGINT CTRL + C and ignores it
    sigfillset(&sigINT.sa_mask);
    sigINT.sa_flags = 0;
    sigaction(SIGINT, &sigINT, NULL);
  }
  /***************************************************************
   *  Function: ignoreSIGSTP
   *  Description: Catches the SIGSTP signal and redirects to sigSTPHandler
   *function to to determine whether foreground will be ignored or not Recieves:
   *struct Outputs:  Node
   ***************************************************************/
  void ignoreSIGSTP(struct sigaction sigSTP) {
    sigSTP.sa_handler =
        sigSTPHandler; // Catches the signal from SIGTSTP CTRL + Z
    sigfillset(&sigSTP.sa_mask);
    sigSTP.sa_flags = 0;
    sigaction(SIGTSTP, &sigSTP, NULL);
  }
  /***************************************************************
   *  Function: clearbuffer
   *  Description: Clears the input buffer and sets it to NULL so that
   *  data can be entered in
   *  Recieves:  char *
   *  Outputs: NONE
   ***************************************************************/
  void clearBuffer(char *inputBuffer[]) {
    int i = 0;
    while (i < 512) {
      inputBuffer[i] = NULL;
      i++;
    }
  }
  /***************************************************************
   * DRIVER CODE
   * Description: 
   ***************************************************************/
  int main() {
    int pid = getpid();
    char *writeMessage;
    clearBuffer(inputBuffer); // clear the input buffer
    ignoreSIGINT(sigINT);     // Ignore SIGINT
    ignoreSIGSTP(sigSTP);     // Catch SIGSTP

    do {
      getuserInput(inputBuffer, pid);
      int res = getCMD(inputBuffer);
      if (res != 0) {
        forkProcess(inputBuffer, sigINT);
      }

      // Reset the fork status
      executeFork = true;

      // Clear the input buffer for next input
      clearBuffer(inputBuffer);

      // Reset the background process and file execution variables
      backgroundProcess = false;
      inputFileFlag = false;
      outputFileFlag = false;

    } while (flag != 0);
    return 0;
  }
