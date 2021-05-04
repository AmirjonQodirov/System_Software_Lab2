#include <string.h>
#include "partitions/partition.h"
#include "hfsp/hfsp.h"

static part parts[200];
static hfsplus* fs;
char buff[1000];

char *show_parts(){
    memset(buff,0,1000);
    uint64_t size = get_parts(parts, 200);
    char tmp[100] = {0};
    sprintf(tmp, " MAJ | MIN | SIZE(Mb) | NAME\n --- | --- | -------- | -----\n");
    strcat(buff, tmp);
    for (int i = 0; i < size; ++i) {
        sprintf(tmp," %3d | %3d | %8lu | %s\n", parts[i].maj, parts[i].min, parts[i].size_mb, parts[i].name);
        strcat(buff, tmp);
    }
    buff[strlen(buff)-1] = '\0';
    return buff;
}

char *read_hfsp(char *filename){
    memset(buff,0,1000);
    fs = read_hfsplus(filename);
    char tmp[100] = {0};
    if (ERR == ERR_RESERVE) {
	    sprintf(tmp, "\033[0;31mError while reading volume header: wrong reserved space\033[0m\n");
	    strcat(buff, tmp);
        buff[strlen(buff)-1] = '\0';
        return buff;
	}
	if (ERR == ERR_WRONG_STRUCTURE) {
	    sprintf(tmp, "\033[0;31mError while reading fs: file system haven't hfs+ structure\033[0m\n");
	    strcat(buff, tmp);
        buff[strlen(buff)-1] = '\0';
        return buff;
	}
    sprintf(tmp, "\033[0;32mDONE!\033[0m\n");
	strcat(buff, tmp);
    buff[strlen(buff)-1] = '\0';
    return buff;
}

char *show_pwd(){
    return fs->pwd;
}

char *show_ls(){
    memset(buff,0,1000);
    int *is_file = malloc(sizeof(int));
    char* bf[1000];
    uint32_t cnt = ls(fs, bf, is_file);
    char tmp[100] = {0};
    sprintf(tmp,"\033[0;33m\n(type) | name\n-----  | ----    \n");
    strcat(buff,tmp);
    for (int i = 0; i < cnt; ++i) {
        if(*is_file == 0){
            sprintf(tmp,"\033[0;34m(d)    | ");
            strcat(buff,tmp);
        }else if(*is_file == 1){
            sprintf(tmp,"\033[0;36m(f)    | ");
            strcat(buff,tmp);
        }
        sprintf(tmp,"%s\n\033[0m",bf[i]);
        strcat(buff,tmp);
    }
    buff[strlen(buff)-1] = '\0';
    return buff;
}

char *do_cd(char *folder){
    memset(buff,0,1000);
    char tmp[100] = {0};
    char *pwd = cd(fs, folder);
    if (pwd == NULL) {
        if (ERR == ERR_CANT_FIND) {
            sprintf(tmp,"\033[0;31mWARNING during execution of command 'cd': No such file or directory\033[0m");
        } else if (ERR == ERR_FIND_FILE) {
            sprintf(tmp,"\033[0;31mWARNING during execution of command 'cd': It is a file\033[0m");
        }
    } else {
        sprintf(tmp,"\033[0;32m%s\033[0m",pwd);
    }
    strcat(buff,tmp);
    buff[strlen(buff)-1] = '\0';
    return buff;
}

char *do_back(){
    memset(buff,0,1000);
    char tmp[100] = {0};
    char *pwd = back(fs);
    if (pwd == NULL && ERR == ERR_ON_ROOT) {
        sprintf(tmp,"\033[0;31mWARNING during execution of command 'back': It is root!\033[0m");
    } else {
        sprintf(tmp,"\033[0;32m%s\033[0m", pwd);
    }
    strcat(buff,tmp);
    buff[strlen(buff)-1] = '\0';
    return buff;
}

char *do_copy(char *file, char *dest){
    memset(buff,0,1000);
    char tmp[100] = {0};
    uint32_t res = copy(file, dest, fs);
    if (res < 0) {
        sprintf(tmp, "\033[0;31mWARNING during execution of command 'copy'\033[0m");
    }else{
        sprintf(tmp,"\033[0;32mDONE!\033[0m");
    }
    strcat(buff,tmp);
    buff[strlen(buff)-1] = '\0';
    return buff;
}