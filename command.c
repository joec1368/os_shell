
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_record();
int lsh_replay(char **args,char **Strinbuffer);
int lsh_echo(char **args);
int lsh_mypid(char **args);
void close_fd(int fd);
int setting();
char* certain_Address = "./buffer.txt";
char* certain_Address2 = "./buffer5.txt";
char* certain_Address3 = "./buffer3.txt";
char* certain_Address4 = "./buffer4.txt";
int saved_stdout ;

//struct command_queue{
//    char **string ;
//    struct command_queue* next;
//    struct command_queue* back;
//};


struct command_queue{
    char **string ;
    char *buffer;
    struct command_queue* next;
    struct command_queue* back;
};

struct command_queue* queueLast;
struct command_queue* queueFirst;
int count_string_in_queue;

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
        "cd",
        "help",
        "exit",
        "record",
        "echo",
        "mypid"

};

int (*builtin_func[]) (char **) = {
        &lsh_cd,
        &lsh_help,
        &lsh_exit,
        &lsh_record,
        &lsh_echo,
        &lsh_mypid,
};

void addCommandInStack(char** c,char * buffer){
    struct command_queue* temp = queueLast;
    struct command_queue* final = queueFirst;
    for(; temp -> next != final ; temp = temp->next);
    free(queueFirst->string);
    //free(queueFirst -> buffer);
    queueFirst->string = malloc(32 * sizeof(char**));
    queueFirst -> next = queueLast;
    queueFirst -> back = NULL;
    queueLast = queueFirst;
    queueFirst = temp;
    temp -> next = NULL;
    queueLast ->next ->back = queueLast;
    count_string_in_queue = 0;
    int i = 0;
    for( ; c[i] != NULL ; i++){
        queueLast -> string[i] = c[i];
    }
    for(; i <31 ; i++) queueLast -> string[i] = NULL;
    queueLast -> buffer = buffer;

    // next 是 從 最後放的 指向 最早放的
    // back 是從 最早放的 指向 最後放的

}

void addCharInStack(char character){
    queueLast -> string[count_string_in_queue++] = character;

}

int lsh_record(){
    struct command_queue* temp;
    int i = 1;
    int count = 0;
    int fd = setting();
    for(temp = queueFirst -> back ; count < 16 ; count++,temp = temp->back){
        printf("%d : ",i++);
        for(int j = 0 ; ; j++){
            if(temp -> string == NULL) {
                printf("\n");
                break;
            }
            else if(temp->string[j] == NULL) {
                printf("\n");
                break;
            }
            printf("%s ",temp->string[j]);
        }
    }
    close_fd(fd);
    return 1;
}

// the order at builtin_str and builtin_func need to be same

//count how many command
int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
    int fd = setting();
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) { // chdir change directory
            perror("lsh");
        }
    }
    close_fd(fd);
    return 1;
}

int lsh_help(char **args)
{
    int i;
    int fd = setting();
    printf("Hi ! I hope that you can have a good time at here\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    printf("1: help :   show all build-in function info\n");
    printf("2. cd :     change directory \n");
    printf("3. echo :   echo the string to standard output\n");
    printf("4. record : shoe last-16 cmds you typed in\n");
    printf("5. replay : re-execute the cmd showed in record\n");
    printf("6. mypid :  find and print process-ids \n");
    printf("7. exit :   exit shell\n");

    printf("Use the man command for information on other programs.\n");

    close_fd(fd);
    return 1;
}

int lsh_echo(char ** args){

    int flag = 0;
    int i = 1;
    int fd = setting();
    if(args[1] != NULL && strcmp(args[1],"-n") == 0) {
        flag = 1;
        i = 2;
    }
    while(1){
        if (args[i] != NULL){
            printf("%s ",args[i++]);
        }else{
            if(flag == 0)
                printf("\n");

            close_fd(fd);
            return 1;
        }
    }


}

int lsh_mypid(char **args){
    int fd = setting();
    if(strcmp(args[1],"-i") == 0){
        int pid = getpid();
        printf("%d\n",pid);
        close_fd(fd);
        return 1;
    }
    if(strcmp(args[1],"-p") == 0){
        if(args[2] == NULL) {
            printf("forget input pid\n");
            close_fd(fd);
            return 1;
        }
        int pid = atoi(args[2]);
        char file[100];
        char status[] = {
                '/',
                's',
                't',
                'a',
                't',
                '\0'
        };
        file[0] = '/';
        file[1] = 'p';
        file[2] = 'r';
        file[3] = 'o';
        file[4] = 'c';
        file[5] = '/';
        int i = 6;
        int y = 0;
        for( ; ; i++) {
            file[i] = args[2][y++];
            if(args[2][y] == '\0') break;
        }
        i++;
        y=0;
        for(;;i++){
            file[i] = status[y++];
            if(status[y] == '\0') break;
        }
        file[++i] = '\0';
        //printf("%s\n",file);
        if (access(file, F_OK) == 0) {//file exists
            FILE *fp = fopen(file, "r");

            int unused;
            char comm[1000];
            char state;
            int ppid;
            fscanf(fp, "%d %s %c %d", &unused, comm, &state, &ppid);
            printf("%d\n", ppid);
            fclose(fp);
        }else{
            printf("file dont exist\n");
        }
        close_fd(fd);
    }
    if(strcmp(args[1],"-t") == 0){
        int pid = getppid();
        printf("%d\n",pid);
        close_fd(fd);
        return 1;
    }
    if(strcmp(args[1],"-c") == 0){
        if(args[2] == NULL) {
            printf("forget input pid\n");
            close_fd(fd);
            return 1;
        }
        char file[100];
        int cpid = atoi(args[2]);
        sprintf(file, "/proc/%d/task/%d/children", cpid, cpid);
        //printf("%s\n",file);
        if (access(file, F_OK) == 0) {//file exists
            FILE *fp = fopen(file, "r");

            char buffer[1000];

            while (fgets(buffer,1000, fp) != NULL)
                printf("%s\n",buffer);
            fclose(fp);
        }else{
            printf("file dont exist\n");
        }
    }
    close_fd(fd);
    return 1;

}

int lsh_exit(char **args)
{
    printf("\n");
    return 0;
}

int setting(){
    FILE *fptr = fopen(certain_Address,"w+");
    fclose(fptr);
    saved_stdout = dup(1);
    int fd = open(certain_Address, O_WRONLY | O_APPEND);
    dup2(fd, 1);
    return fd;
}

void close_fd(int fd){
    close(fd);
    FILE *fptr = fopen(certain_Address,"a+");
    if (!fptr) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    fputc(NULL,fptr);
    fclose(fptr);
    dup2(saved_stdout, 1);
}