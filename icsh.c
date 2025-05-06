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
#include "signal.h"
#include "fcntl.h"

#define MAX_CMD_BUFFER 255

volatile sig_atomic_t pid_foreground = 0; 
//volatile to avoid unexpected changes from a handler
//sig_atomic_t is a type of integer that can be safely read/written in a signal handler
//called pid_foreground because it is meant to track the ID of the child process currently running in the foreground.

int latest_exit_status = 0; //tracks latest command's exit status to support echo $? (part of Milestone 4)



void signint_handler(int signal){ //int signal is not used, but signal() expects handlers to be defined this way.
    if (pid_foreground > 0){ //pid_foreground > 0 means that there IS a foreground process running. This if statement is to make sure the signal only affects the foreground process.
        kill(pid_foreground, SIGINT); //kill actually sends a signal and not *kill* something. In this case, sigint will be handled by sending SIGINT to the child process (the foreground process).
    }
}

void sigtstp_handler(int signal){
    if (pid_foreground > 0){
        kill(pid_foreground, SIGTSTP); //similar to above but sends SIGTSTP instead
    }
}

//As of Milestone 5 parsing command is now handled here.
void parse_command(char *line, char **args, char **input_file, char **output_file){
    *input_file = NULL;
    *output_file = NULL;

    int i = 0;
    char *token = strtok(line, " ");
    while (token != NULL){
        if (strcmp(token, "<") == 0){ //if we find a redirection
            token = strtok(NULL, " ");
            *input_file = token; //the next token after the redirection symbol would be the file name
        }
        else if (strcmp(token, ">") == 0){ //handled same way as above input redirection
            token = strtok(NULL, " ");
            *output_file = token;
        }
        else{
            args[i++] = token; //if not a redirection then it's just more arguments we store into args
        }
        token = strtok(NULL, " "); //move to next token to prep for next iteration of the while loop
    }
    args[i] = NULL; //end args with NULL because execvp requires it
}

//Milestone 5: Redirection helper functions
int original_stdin = -1;
int original_stdout = -1;

void apply_redirection(char *input_file, char *output_file){
    original_stdin = dup(STDIN_FILENO); //when we apply redirection, we will save the original stdin and stdout so we can restore it for parent process
    original_stdout = dup(STDOUT_FILENO);

    //Milestone 5: Handling redirection
    if (input_file){ //this block runs if we have an input file
        int file_in = open(input_file, O_RDONLY); //open the input file
        if (file_in < 0){ //open will return -1 if opening the file fails
            perror("Input redirection failed");
            exit(1);
        }
        
        dup2(file_in, STDIN_FILENO); //if the input file was opened successfully, we will replace STDIN with the input file instead
        close(file_in); //close after done
    }

    if (output_file){ //this block runs if we have an output file/intend to create a new output file
        int file_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); //open output file to write or create if the given output file does not already exist. 0644 is the permission typical for a document file like this.
        if (file_out < 0){
            perror("Output redirection failed");
            exit(1);
        }

        dup2(file_out, STDOUT_FILENO); //replace the STDOUT with writing into the output file instead
        close(file_out); //close after done
    }

}

void restore_redirection(){  //reverse the redirection. Meant to restore stdin and stdout for the parent process in case of built in commands like echo
    if (original_stdin != -1){ //wont run if there was no redirection in the first place
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);
        original_stdin = -1;
    }

    if (original_stdout != -1){
        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
        original_stdout = -1;
    }
}



//As of milestone 2 main now accepts arguments
int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char latest_command[MAX_CMD_BUFFER] = "";  //This line will help store the latest command from the user
    FILE *input = stdin;


    if (argc == 2){
        input = fopen(argv[1], "r");
        if (!input){
            perror("Could not open given file");
            exit(1);
        }
    }

    //If user sends in SIGINT or SIGTSTP, they are caught here and handled by handlers above
    signal(SIGINT, signint_handler);
    signal(SIGTSTP, sigtstp_handler);

    if (input == stdin){ //updated to only print welcome message in interactive mode and not for script mode
        printf("Welcome to IC Shell!\n"); 
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
        if (strlen(buffer) == 0 || buffer[0] == '#' || (buffer[0] == '/' && buffer[1] == '/')){ //if the first character is '#' or first two being "//" then that means it's a comment and we will just skip
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

        if (strcmp(buffer, "echo $?") == 0){  //special case of echo $? command for Milestone 4
            printf("%d\n", latest_exit_status);
            continue;
        }



        char *args[MAX_CMD_BUFFER / 2 + 1];
        char *input_file = NULL;
        char *output_file = NULL;

        parse_command(buffer, args, &input_file, &output_file);
        char *command = args[0];

        
        if (args[0] == NULL){
            continue; //if input is empty we just reprompt. 
        }

        for (int j = 0; args[j] != NULL; j++){ //iterate through the args to check if we have either < or > or not. 
            if (strcmp(args[j], "<") == 0 && args[j+1] != NULL){ //finding a < means it is an input redirection
                input_file = args[j+1]; //the correct format of input dictates that the next argument after the < will be the file
                args[j] = NULL; //we then replace the < with NULL to truncate the command to be executed by execvp at the redirection
            }
            else if (strcmp(args[j], ">") == 0 && args[j+1] != NULL){ //similar to above but if we find a > then it is an output redirection
                output_file = args[j+1]; 
                args[j] = NULL;
            }
        }


        //Adding in echo command
        if (strcmp(command, "echo") == 0){
            apply_redirection(input_file, output_file); //apply redirection if needed

            for (int i = 1; args[i] != NULL; i++){
                printf("%s", args[i]);
                if (args[i+1] != NULL){
                    printf(" ");
                }
            }
            printf("\n");
            
            latest_exit_status = 0; //for Milestone 4: all built-in commands expected to exit with exit code 0 in normal circumstances

            restore_redirection(); //reverse the redirection if needed
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
            latest_exit_status = exit_code;
            exit(exit_code);

        }
        //Handling invalid commands used to be here. As of milestone 3, non built-in commands are now expected to be external commands.
        else {
            
            
            for (int j = 0; args[j] != NULL; j++){ //iterate through the args to check if we have either < or > or not. 
                if (strcmp(args[j], "<") == 0 && args[j+1] != NULL){ //finding a < means it is an input redirection
                    input_file = args[j+1]; //the correct format of input dictates that the next argument after the < will be the file
                    args[j] = NULL; //we then replace the < with NULL to truncate the command to be executed by execvp at the redirection
                }
                else if (strcmp(args[j], ">") == 0 && args[j+1] != NULL){ //similar to above but if we find a > then it is an output redirection
                    output_file = args[j+1]; 
                    args[j] = NULL;
                }
            }


            pid_t pid = fork();
            if (pid < 0){ //fork() will return -1 if an error occurs
                perror("Forking failed");
                continue;
            }
            else if (pid == 0){  //fork() will return 0 in the child process
                //therefore, this else if block is inside the child process


                apply_redirection(input_file, output_file); //apply the redirection if there is a need to


                execvp(command, args);  //if execvp *doesn't* fail then nothing after this line will run
                perror("execvp failed"); //as such, if this line does get executed, we know for sure execvp went wrong
                exit(1);
            }
            else{
                pid_foreground = pid; //the PID here is the process ID of the child process so we update the pid_foreground here
                int status;
                waitpid(pid, &status, WUNTRACED); //waitpid will write the child's exit status into the status variable. As of Milestone 4, "0" has been changed to "WUNTRACED". This is because the parent process simply waiting for the child process to finish as indicated by "0" is no longer sufficient. Instead, the child process can now possibly be stopped as well, and the WUNTRACED accomodates that.
                pid_foreground = 0; //by this point there is no child process left. As such, we set pid_foreground = 0 so that the Milestone 4 signals will not affect beyond the foreground job.

                if (WIFEXITED(status)) { //TRUE if child process exited normally
                    latest_exit_status = WEXITSTATUS(status); //WEXITSTATUS extracts the status code from "status" so we can keep track of it. Supposed to be 0 if the child process was terminated with no error and 1 if there was an error.
                }
                else if (WIFSIGNALED(status)){ //TRUE if child process exited because of a signal
                    latest_exit_status = 128 + WTERMSIG(status); //WTERMSIG extracts which signal caused the child process to exit. By convention we also add 128 to this number.
                }
                else if (WIFSTOPPED(status)){ //TRUE if child process stopped because of a signal
                    latest_exit_status = 128 + WSTOPSIG(status); //WSTOPSIG extracts which signal caused the child process to *stop*. Similar convention to above
                }
                else{
                    latest_exit_status = 1; //exit code of 1 indicating error
                }
            }
        }
        
    }
    if (input != stdin){ //closing file 
        fclose(input);
    }
    return 0;
}
