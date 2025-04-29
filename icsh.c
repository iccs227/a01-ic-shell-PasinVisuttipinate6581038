/* ICCS227: Project 1: icsh
 * Name: Pasin Visuttipinate (Ryo)
 * StudentID: 6581038
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/wait.h"

#define MAX_CMD_BUFFER 255

//As of milestone 2 main now accepts arguments
int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char latest_command[MAX_CMD_BUFFER] = "";  //This line will help store the latest command from the user
    FILE *input = stdin;

    printf("Welcome to IC Shell!\n");

    if (argc == 2){
        input = fopen(argv[1], "r");
        if (!input){
            perror("Could not open given file");
            exit(1);
        }
    }

    while (1) {
        if (input == stdin){ //This if statement is for script mode. If we were in script mode, there would be no need to print "icsh $"
            printf("icsh $ ");
            //fgets(buffer, 255, stdin);   If we weren't including the Crtl + D feature I would uncommnet this line
            fflush(stdout);  //fflush makes sure the message "icsh $ " gets printed immediately to avoid the potential confusion that the shell is frozen
        }
        
        //This isnt necessary for milestone 1, but allows user to quit the shell with Crtl + D
        if (fgets(buffer, MAX_CMD_BUFFER, input) == NULL) { 
            if (input != stdin){
                break;
            }
            else {
                printf("\n");
                break;
            }
            
        }
    
        
        //Removes the newline character from the user input and replace it with the null terminator.
        //This is meant to help make, for example, strcmp(buffer, "exit") work as expected.
        //strcspn finds "\n" index from within buffer. Then we just replace that index with '\0'. 
        buffer[strcspn(buffer, "\n")] = '\0'; 

        //Handling empty input from user by just prompting again
        if (strlen(buffer) == 0 || buffer[0] == '#'){ //if the first character is '#' then that means it's a comment and we will just skip
            continue;
        }


        //Adding in !! command
        if (strcmp(buffer, "!!") == 0){
            if (strlen(latest_command) == 0){
                //There is no previous command so we do nothing
                continue;
            }
            else{
                printf("%s\n", latest_command); //print out the latest command for the user
                strcpy(buffer, latest_command); //reaplce buffer with that latest command
            }
        }
        else {
            strcpy(latest_command, buffer);  //Whenever the input isn't "!!" we save that as the latest command
        }

        char *command = strtok(buffer, " ");  //split the input based on the space, effectively storing the command input into *command
        if (command == NULL){
            continue; //for handling when there is no command, we will simply skip the line
        }

        //Adding in echo command
        if (strcmp(command, "echo") == 0){
            char *args = strtok(NULL, ""); //store the rest of the line into *args with no further splitting
            if (args){
                printf("%s\n", args);
            }
        }
        //Adding in exit command
        else if (strcmp(command, "exit") == 0){
            char *arg = strtok(NULL, " ");
            int exit_code = 0;
            if (arg){
                exit_code = atoi(arg); //convert the arg string into integer
                exit_code = exit_code & 0xFF; 
                //truncate the exit code to 8 bits as instructed. Done by using bitwise AND
                //in combination with 0xFF(11111111) to keep only the lowest 8 bits.
            }

            printf("Goodbye!\n");
            exit(exit_code);

        }
        //Handling invalid commands used to be here. As of milestone 3, non built-in commands are now expected to be external commands.
        else {
            char *args[MAX_CMD_BUFFER / 2 + 1];  
            int i = 0;
            args[i++] = command;  

            char *arg;
            while ((arg = strtok(NULL, " ")) != NULL){
                args[i++] = arg; //each token is added into this array of arguments
            }

            args[i] = NULL;  //execvp expects a NULL at the end of arguments

            pid_t pid = fork();
            if (pid < 0){ //fork() will return -1 if an error occurs
                perror("Forking failed");
                continue;
            }
            else if (pid == 0){  //fork() will return 0 in the child process
                //therefore, this else if block is inside the child process
                execvp(command, args);  //if execvp *doesn't* fail then nothing after this line will run
                perror("execvp failed"); //as such, if this line does get executed, we know for sure execvp went wrong
                exit(1);
            }
            else{
                int status;
                waitpid(pid, &status, 0); //waitpid will write the child's exit status into the status variable. 0 indicates that we just want the parent to wait until the child process is finish.
                
            }
        }
        
    }
    if (input != stdin){ //closing file 
        fclose(input);
    }
    return 0;
}
