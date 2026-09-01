#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
//notas do monitor: fork é a coisa mais importante dessa matéria

char** read_command(char* command, int* parameters);

int main(void) {
    char command[100];
    int parameters = 0;
    int status;

    char cwd[1024];
    while(1){
        getcwd(cwd, sizeof(cwd));
        printf("%s%% ", cwd);
        fgets(command, 100, stdin);
        
        char** argv = read_command(command, &parameters);
        for (int k = 0; k <= parameters; k++) {
            //printf("argv[%d] = \"%s\"\n", k, argv[k]);
        }

        if (fork() != 0){
            /*Parent code*/
            waitpid(-1, &status, 0);
        }else{
            /*Child code*/
            execvp(argv[0], argv);
        }

        parameters = 0;
    }
}

char** read_command(char* command, int* parameters) {   
    int i = 0; 
    int j = 0; 
    char** argv = (char**) malloc(sizeof(char*)*100);
    argv[0] = (char*) malloc(sizeof(char*)*20);
    
    while(command[i] != '\0') {
        if (command[i] == ' ') {
            argv[*parameters][j] = '\0';
            (*parameters)++;
            j = 0;
            argv[*parameters] = (char*) malloc(sizeof(char*)*20);
        }else{
            argv[*parameters][j] = command[i];
            if (command[i] == '\n') argv[*parameters][j] = '\0';
            j++;
        }
        i++;
    }

    argv[*parameters + 1] = NULL;


    return argv;
}