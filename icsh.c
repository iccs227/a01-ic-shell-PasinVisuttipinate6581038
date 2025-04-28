/* ICCS227: Project 1: icsh
 * Name: Pasin Visuttipinate (Ryo)
 * StudentID: 6581038
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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
        //Handling invalid commands
        else {
            printf("bad command\n");
        }
        
    }
    if (input != stdin){ //closing file 
        fclose(input);
    }
    return 0;
}
