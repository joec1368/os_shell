#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "command.c"
#include <sys/wait.h>

#define BUFFERSIZE 1024
#define STRINGBUFFERSIZE 128
#define TOKENENDCHAR " \t\r\n\a "

int* count_special_signal(char** StringBuffer);
void read_file(char *args);
char*** package(char **args);
void send_to_file(char *args);

//struct command_queue{
//    char *string ;
//    struct command_queue* next;
//};
//
//extern struct command_queue* queueLast; // 剛放進來
//
//extern struct command_queue* queueFirst; // 要被堆出去

int stack_position = 0;
char buffer[1024] = { 0 };
int pfd[2];
int flag_vetrical_line = 0;
int flag_file_in = 0;
int flag_file_out = 0;
int have_data = 0;
int flag_not_execute = 0;
int child_flag = 0;
int count = 0;



void initialize(){
    struct command_queue* pre;
    struct command_queue* temp;
    char null_array[5] = "NULL";
    for(int i = 0 ; i < 17 ; i++){
        temp = malloc(sizeof( struct command_queue) );
        if(i==0){
            temp->next = NULL;
            temp->string =  malloc(BUFFERSIZE * sizeof(char**));
            temp -> string[0] =  null_array;
            queueFirst = temp;
            pre = temp;
        } else{
            temp -> next = pre;
            temp->string =  malloc(BUFFERSIZE * sizeof(char**));
            temp -> string[0] = null_array;
            pre -> back = temp;
            pre = temp;
        }
    }
    temp -> back = NULL;
    queueLast = temp;
    // next 是 從 最後放的 指向 最早放的
    // back 是從 最早放的 指向 最後放的

    /* create pipe */
    pipe(pfd);
}

int launch(char **buffer){
    pid_t pid, wpid;
    int status;

//    if(flag_vetrical_line == 1){
//        int i = 0 ;
//        while(buffer[i] != NULL) i++;
//        buffer[i] = certain_Address;
//        buffer[i+1] = NULL;
//    }

    pid = fork();


    if (pid == 0) {
        // Child process

//        FILE *fptr = fopen(certain_Address,"r");
//        for(int z = 0 ; z < 4096 ; z++){
//            char ch = fgetc(fptr);
//            if(ch == EOF) break;
//            fprintf(stderr,"%c",ch);
//        }
//        fclose(fptr);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[0]);

        if (execvp(buffer[0], buffer) == -1) {
            perror("lsh");
        }

        close(pfd[0]);
        close(pfd[1]);
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else {
        // Parent process
        do {

            wpid = waitpid(pid, &status, WUNTRACED);
            //We use waitpid() to wait for the process’s state to change.
            //WUNTRACED The status of any child processes specified by pid that are stopped,
            // and whose status has not yet been reported since they stopped,
            // shall also be reported to the requesting process.
            have_data = 1;


        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        //WIFEXITED(status) 这个宏用来指出子进程是否为正常退出的，如果是，它会返回一个非零值。
        //WIFSIGNALED(status)为非0 表明进程异常终止。
        FILE *fptr;
        int len;
        char* temp_for_file = NULL;
        temp_for_file = malloc(sizeof (char )*1024);
        close(pfd[1]);

        int err = remove(certain_Address);

        int fd = open(certain_Address, O_WRONLY | O_TRUNC , S_IRWXU | S_IRWXG | S_IRWXO);
//        fptr = fopen(certain_Address,"w");
//        len=read(pfd[0], temp_for_file, 1024);
//        write(fd,temp_for_file,1024);

        close(fd);

        fptr = fopen(certain_Address,"w+");
        len = read(pfd[0], temp_for_file,1023);
        for(int i = 0 ; i < len ; i++){
            if(temp_for_file[i] == '\0' || temp_for_file[i] == EOF || temp_for_file[i] == NULL) break;
            fputc(temp_for_file[i],fptr);
            //fprintf(stderr,"%c",temp_for_file[i]);
        }

        //fputc(EOF,fptr);
        fclose(fptr);
        free(temp_for_file);
        close(pfd[0]);
        close(pfd[1]);
    }
    return 1;
}

int execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    pfd[1] = 16;
    pfd[0] = 15;
    pipe(pfd);
    return launch(args);
}

char* read_line(){
    int buffersize = BUFFERSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * buffersize);
    char c;

    while(1){
        c = getchar();

        if(c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        }else{
            buffer[position++] = c;
            //addCharInStack(c);
        }

        if(position >= buffersize){
            buffersize += BUFFERSIZE;
            buffer = realloc(buffer,buffersize);
        }
    }
}

char** split_line(char* buffer){
    int buffersize = STRINGBUFFERSIZE;
    int position = 0;
    char **token_array = malloc(buffersize * sizeof(char*));
    char *token;

    token = strtok(buffer,TOKENENDCHAR);
    for(int i = 0 ; i < 32 ; i++){
        token_array[i] = malloc(sizeof (char ) * 64);

    }

    while(token != NULL){
        strcpy(token_array[position++],token);

        if (position >= buffersize) {
            buffersize += STRINGBUFFERSIZE;
            token_array = realloc(token_array, buffersize * sizeof(char*));
        }
        token = strtok(NULL, TOKENENDCHAR);
    }

    token_array[position] = NULL;
    for(; position < buffersize ; position++) token_array[position] = NULL;
    return token_array;
}

void read_and_parse(){
    char *line;
    char **StringBuffer;
    int status;
    do{
        start:
        if(child_flag == 1) return;
        printf(">>>$ ");
        line = read_line();
        StringBuffer = split_line(line);
        if(strcmp(line,"")  != 0 && strcmp(line," ")  != 0 && strcmp(StringBuffer[0],"replay") == 0){
            int temp = atoi(StringBuffer[1]);
            if(temp > 16 || temp <= 0) {
                printf("replay: wrong args\n");
                goto start;
            }
        }
        addCommandInStack(StringBuffer,line);


        int *count = count_special_signal(StringBuffer);
        /*
         *  if(strcmp(StringBuffer[i],"<") == 0) array[0]++;
            else if (strcmp(StringBuffer[i],"|") == 0) array[1]++;
            else if((strcmp(StringBuffer[i],">") == 0)) array[2]++;
            else if(strcmp(StringBuffer[i],"&") == 0) array[3]++;
            array[4]++;i++;
         *
         */
        if(count[4] != 0){
            char*** pack = NULL;
            pack = package(StringBuffer);
            int i = 0;
            background_start:

            i = 0;
            if(StringBuffer != NULL && strcmp(pack[0][0],"replay") == 0){
                int none_use = lsh_replay(pack[0],StringBuffer);
                pack = package(StringBuffer);
                count = count_special_signal(StringBuffer);
            }

            while(pack[i] != NULL){
                if(count[3] != 0){
                    int pid = fork();
                    //printf("%d\n", pid);
                    //fflush(stdout);
                    if (pid == 0) {// Child process
                        count[3] = 0;
                        int z = 0;
                        while (strcmp(pack[z][0],"&")!=0) z++;
                        pack[z][0] = "exit";
                        pack[z][1] = "exit";
                        pack[z + 1][0] = "exit";
                        pack[z+1][1] = NULL;
                        pack[z+2] = NULL;
                        certain_Address = certain_Address2;
                        child_flag = 1;
                        goto background_start;
                    }
                    else{
                        int fd = setting();
                        printf("[pid] = %d\n",pid);
                        close_fd(fd);
                        break;
                    }
                }
                if(pack[1] != NULL && strcmp(pack[1][0],"<") == 0){
                    flag_file_in = 1;
                    read_file(pack[2][0]);
                    int j = 0;
                    while(pack[0][j] != NULL) j++;
                    pack[0][j] = certain_Address;
                    pack[0][j+1] = NULL;
                    pack[0][j+2] = NULL;
                    j = 1;
                    while(pack[j+2] != NULL){
                        pack[j] = pack[j+2];
                        j++;
                    }
                    pack[j] = NULL;
                    pack[j+1] = NULL;
                }
                if(pack[i] != NULL && strcmp(pack[i][0],"|") == 0 ){
                    flag_vetrical_line = 1;
                    i++;
                    if(flag_vetrical_line == 1){
                        int j = 0 ;
                        while(pack[i][j] != NULL) j++;
                        pack[i][j] = certain_Address;
                        pack[i][j+1] = NULL;
                    }
                    for(int z = 0 ;  z < 5 ; z++){
                        if(pack[z] == NULL) continue;
                        for(int j = 0 ; j < 5 ; j++){
                            if(pack[z][j] == NULL) break;
                        }
                    }
                }
                if(pack[i] != NULL && strcmp(pack[i][0],">") == 0){
                    send_to_file(pack[i+1][0]);
                    remove(certain_Address);
                    goto start;
                }
                status = execute(pack[i++]);
                if(status == 0 && child_flag == 0) return;
            }
            char* file_array;
            file_array = malloc(sizeof (char) * 1024);
            FILE *fptr = fopen(certain_Address,"r");
            for(int z = 0 ; z < 4096 ; z++){
                char ch = fgetc(fptr);
                if(ch == NULL || ch == EOF) break;
                printf("%c",ch);
            }
            fclose(fptr);
            have_data = 0;
            remove(certain_Address);
            if(child_flag == 1) return;
        }
        //free(line);
        free(StringBuffer);
        free(count);
    } while (status);

}

int* count_special_signal(char** StringBuffer){
    int* array = malloc(sizeof (int) * 5);
    for(int i = 0 ; i < 5 ; i++) array[i] = 0;
    int i = 0;
    while(StringBuffer[i] != NULL){
        if(strcmp(StringBuffer[i],"<") == 0) array[0]++;
        else if (strcmp(StringBuffer[i],"|") == 0) array[1]++;
        else if((strcmp(StringBuffer[i],">") == 0)) array[2]++;
        else if(strcmp(StringBuffer[i],"&") == 0) array[3]++;

        array[4]++;i++;
    }
    return array;
}

int lsh_replay(char **args,char **Strinbuffer){
    int number = atoi(args[1]) - 1 ;
    struct command_queue* temp = queueLast;
    for(int i = 16 ; i != number ;i--){
        temp = temp -> next;
    }

    int i = 0;
    while (temp->string[i] != NULL) i++;

    char** copy = malloc((i+32)* sizeof (*temp->string));


    int j = 0;
    for(; j < i; j++)
    {
        copy[j] = malloc(strlen(temp->string[j]) + 32);
        strcpy(copy[j], temp->string[j]);
    }
    if(queueLast->string[2] != NULL){
        int z = 2;
        while(queueLast->string[z] != NULL){
            copy[j] = malloc(strlen(queueLast->string[z]) + 5);
            strcpy(copy[j], queueLast->string[z]);
            z++;j++;
        }
    }
    copy[j] = NULL;

    free(queueLast -> string);
    free(queueLast -> buffer);
    queueLast -> string = copy;
    queueLast -> buffer = copy;
    int status = 1;
    int z = 0;
    for(; ;z++){
        if(Strinbuffer[z] == NULL){
            Strinbuffer[z] = malloc(sizeof (char*) * 64);
        }
        if(copy[z] != NULL){
            strcpy(Strinbuffer[z],copy[z]);
        }else{
            Strinbuffer[z] = NULL;
            break;
        }
    }
    for(;z < 64 ; z++) Strinbuffer[z] = NULL;
//    if(flag_not_execute == 0)
//         status = execute(temp -> string);
//    else {
//        flag_not_execute = 0;
//    }
    return status;
}

char*** package(char **args){
    int i = 0 ;
    int array_pointer = 0;
    int sub_array_count = 0;
    char*** array = malloc(sizeof (char**) * 64);
    for(i = 0 ; i < 64 ; i++){
        array[i] = malloc(sizeof (char*) * 64);
    }
    i = 0;
    while(args[i] != NULL && args[i] != '\0'){
        if(strcmp(args[i],"|") == 0 || strcmp(args[i],">") == 0 || strcmp(args[i],"<") == 0 || strcmp(args[i],"&") == 0){
            array[array_pointer++][sub_array_count] = NULL;
            array[array_pointer][0] = args[i++];
            array[array_pointer][1] = NULL;
            sub_array_count = 0;
            array_pointer++;
        }else{
            array[array_pointer][sub_array_count++] = args[i++];
        }
    }
    for(;sub_array_count < 64 ; sub_array_count++)  array[array_pointer][sub_array_count] = NULL;
    array[++array_pointer] = NULL;
    for(;array_pointer < 64 ; array_pointer++) array[array_pointer] = NULL;
    return array;
}

void read_file(char *args){
    FILE *fptr;
    int len;
    remove(certain_Address);
    fptr = fopen(args,"r");
    FILE *fptr2 = fopen(certain_Address,"w+");
    for(int z = 0 ; z < 1024 ; z++){
        char ch = fgetc(fptr);
        if(ch == '\0' || ch == EOF || ch == NULL) break;
        fputc(ch,fptr2);
    }
    fclose(fptr2);
    fclose(fptr);
}

void send_to_file(char *args){
    FILE *fptr;
    int len;
    fptr = fopen(certain_Address,"r");
    FILE *fptr2 = fopen(args,"w+");
    for(int z = 0 ; z < 1024 ; z++){
        char ch = fgetc(fptr);
        if(ch == '\0' || ch == EOF || ch == NULL) break;
        fputc(ch,fptr2);
    }
    fclose(fptr2);
    fclose(fptr);
}

int main() {
    printf("Hi welcome \n");
    printf("Hope you have a good time \n");
    printf("Feel free to use help to check build-in command \n");
    initialize();
    read_and_parse();
    return 0;
}
