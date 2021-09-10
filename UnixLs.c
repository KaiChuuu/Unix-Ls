
#include <unistd.h>  //for close()
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <locale.h>
#include <stdint.h>
#include <dirent.h>
#include <langinfo.h>
#include <string.h> //for strncmp()
#include <time.h> //for ctime()

DIR* dir_p;
struct dirent* dirent_p;

char output[1024][1024];
int output_index;

char directory[1024][1024];
int directory_index;

char directory_R[1024][1024];
int directory_R_index;

struct commands {
    bool option_i;
    bool option_R;
    bool option_l;
} temp, real;

//Lock in options
bool lock;

//true = If its a folder 
//false = If its something else
bool add_R;

//Whether its a option or a directory
//true = option
//false = directory
bool cases;

//request type
//true -> directory 
//false -> file / symbolic link
bool req_type;

struct stat stats;
char value[1024];
char cur_time[1024];

char read_sym_link[1024];

char temp_string[1024];

//strcmp  0 is true , 1 is false

void st_mode_to_rwx(int st_mode){

    //No permissions
    strcpy(value, "---------- ");

    //Dir
    if(S_ISDIR(st_mode)){
        value[0] = 'd';   
    }  
    //Char Dev
    if(S_ISCHR(st_mode)){
        value[0] = 'c';   
    } 
    //Block Dev
    if(S_ISBLK(st_mode)){
        value[0] = 'b';    
    }
    //Link
    if(S_ISLNK(st_mode)){
        value[0] = 'l';
    }

    //User
    if(st_mode & S_IRUSR){
        value[1] = 'r'; 
    }   
    if(st_mode & S_IWUSR){
        value[2] = 'w';
    }
    if(st_mode & S_IXUSR){
        value[3] = 'x';
    }

    //Group
    if(st_mode & S_IRGRP){
        value[4] = 'r';    
    }
    if(st_mode & S_IWGRP){
        value[5] = 'w';
    }
    if(st_mode & S_IXGRP){
        value[6] = 'x';
    }
    
    //Rest
    if(st_mode & S_IROTH){
        value[7] = 'r';   
    }
    if(st_mode & S_IWOTH){
        value[8] = 'w';
    } 
    if(st_mode & S_IXOTH){
        value[9] = 'x';
    }


}

void argument_i(){
    
    sprintf(value, "%ld", stats.st_ino);
    strcat(value, " ");
    strcat(output[output_index], value);

}

void argument_l(){
    
    memset(value, 0, strlen(value));
    memset(cur_time, 0, strlen(cur_time));

    //st_mode
    st_mode_to_rwx(stats.st_mode);
    strcat(output[output_index], value);    

    //st_nlink
    sprintf(value, "%-2ld ", stats.st_nlink);
    strcat(output[output_index], value);

    struct passwd *pwd;
    if((pwd = getpwuid(stats.st_uid)) != NULL){
        sprintf(value, "%-8s ", pwd->pw_name);
    }else{
        sprintf(value, "%-8d ", stats.st_uid);
    }
    strcat(output[output_index], value);
    
    struct group *grp;
    if((grp = getgrgid(stats.st_gid)) != NULL){
        sprintf(value, "%-8s ", grp->gr_name);
    }else{
        sprintf(value, "%-8d ", stats.st_gid);
    }
    strcat(output[output_index], value);

    sprintf(value, "%5ld ", stats.st_size);
    strcat(output[output_index], value);

    struct tm *timeinfo;
    timeinfo = localtime(&stats.st_mtime);
    strftime(cur_time, 1024, "%b %e %Y %H:%M ", timeinfo);
    strcat(output[output_index], cur_time);

}



void create_line(){    
    if(real.option_i){
        argument_i();
    }
    if(real.option_l){
        argument_l();
    }
    if(real.option_R){
        //Dir
        if( S_ISDIR(stats.st_mode)){
            add_R = true;
        } 
    }

    if(!req_type){
        strcat(output[output_index], dirent_p->d_name);
        strcat(output[output_index], "  ");
        output_index++;
    }
    
}

void handle_symbolic_link(){
    if(lstat(dirent_p->d_name, &stats) == -1){
        printf("ls: cannot access '%s'\n", dirent_p->d_name);
    }
    create_line();
    
    if(real.option_l){
        strcat(output[output_index-1], " -> ");
        strcat(output[output_index-1], read_sym_link);
    }
}

void ls_classic(int dir_index){

    dir_p = opendir(directory[dir_index]);
    if(dir_p == NULL){
        if(stat(directory[dir_index], &stats) == -1){
            printf("ls: cannot access '%s': No such file or directory\n", directory[dir_index]);
            return;   
        }else{
            ///file go in here
          
            req_type = true;
            create_line();
            strcat(output[output_index], directory[dir_index]);
            strcat(output[output_index], "  ");
            output_index++;
            
            return;
        }
    }

    for(;;){
        dirent_p = readdir(dir_p);
        if(dirent_p == NULL){
            break;
        }
        if(dirent_p->d_name[0] == '.'){
            //It is a hidden file/folder
            //Do not read
            continue;
        }

        strcpy(temp_string, directory[dir_index]);
        if(directory[dir_index][strlen(directory[dir_index])-1] != '/'){
            strcat(temp_string, "/");
        }
        strcat(temp_string, dirent_p->d_name);
        if(stat(temp_string, &stats) == -1){
            if(readlink(dirent_p->d_name, read_sym_link, sizeof(read_sym_link)) != -1 ){
                //Symbolic link
                handle_symbolic_link();
            }else {      
                continue;
            }

        }else if(!strcmp(dirent_p->d_name,".") || !strcmp(dirent_p->d_name,"..")){
            //Do nothing
            continue;
        }else{
            create_line();
            //readlink() over here accounts for symbolic folders
            if(real.option_R && add_R && readlink(temp_string, read_sym_link, sizeof(read_sym_link)) == -1){
                strcat(directory_R[directory_R_index], directory[dir_index]);
                if(directory[dir_index][strlen(directory[dir_index])-1] != '/'){
                    strcat(directory_R[directory_R_index], "/");
                }
                strcat(directory_R[directory_R_index], dirent_p->d_name);
                directory_R_index++;
                add_R = false;
            }

        }
    }
    closedir(dir_p);
}

void initializeVariables(){
    
    lock = false;
    cases = false;
    req_type = false;
    add_R = false;
        

    real.option_i = false;
    real.option_l = false;
    real.option_R = false;
    temp.option_i = false;
    temp.option_l = false;
    temp.option_R = false;

    directory_index = 0;
    output_index = 0;
    directory_R_index = 0;
    
}

int main(int argc, char *argv[]) {
    //argc is number of items
    //argv from 0->~ is items   
    initializeVariables();

    //No extra arguments
    if(argc == 1){
        strcpy(directory[directory_index], ".");    
        directory_index++;    
    }else{
        for(int i = 1; i<argc; i++){
            cases = false;
            if(argv[i][0] == '-'){
                
                //Argument must be less than 4
                if(strlen(argv[i]) <= 4 && strlen(argv[i]) > 1){
                    lock = false;
                    for(int y = 1; y< strlen(argv[i]); y++){
                        if(argv[i][y] == 'i'){
                            temp.option_i = true;
                        }else if(argv[i][y] == 'R'){
                            temp.option_R = true;
                        }else if(argv[i][y] == 'l'){
                            temp.option_l = true;
                        }else{
                            //its not a option 
                            //(i.e the command is invalid)
                            temp.option_i = false;
                            temp.option_R = false;
                            temp.option_l = false;
                            lock = true;
                            break;
                        }
                    }
                    if(lock == false){
                        cases = true;
                        if(temp.option_i == true){
                            real.option_i = temp.option_i;
                        }
                        if(temp.option_R == true){
                            real.option_R = temp.option_R;
                        }
                        if(temp.option_l == true){
                            real.option_l = temp.option_l;
                        }
                    }
                }           
            }
            //has to be a directory 
            if(cases == false){
                strcpy(directory[directory_index], argv[i]);
                directory_index++;        
            } 
        }
    }

    if(directory_index == 0){
        //Must set directory to current
        strcpy(directory[directory_index], ".");
        directory_index++;
    }
    


    // printf("directories\n");
    // for(int p =0; p<directory_index; p++){
    //     printf("%s ",directory[p]);
    // }
    // printf("\ni : %d",real.option_i);
    // printf("R : %d",real.option_R);
    // printf("l : %d",real.option_l);
    


    do{
        directory_R_index = 0;
        //Multiple directories
        //add option variants to output
        for(int w =0; w<directory_index; w++){
            ls_classic(w);
            
            if(real.option_R){
                printf("%s:\n", directory[w]);
            }else if(directory_index > 1){
                if(!req_type){
                    printf("%s:\n", directory[w]);
                }
            }
            //print ls results
            for(int r =0; r<output_index; r++){
                printf("%s\n", output[r]);
            }
            
            for(int f =0; f<output_index; f++){
                strcpy(output[f], "");
            }
            output_index = 0;
            req_type = false;

            if(w != directory_index-1){
                printf("\n");
            }
        }

        if(real.option_R && directory_R_index >= 1){
            printf("\n");
            
            for(int g =0; g<directory_index; g++){
                strcpy(directory[g], "");
            }

            //Set next directory values while clearing directory_R
            for(int u =0; u<directory_R_index; u++){
                strcpy(directory[u],directory_R[u]);
                strcpy(directory_R[u], "");
            }
            directory_index = directory_R_index;
        }

    //if option -R, go into next folder
    }while(real.option_R && directory_R_index >= 1);
   
  

    
}