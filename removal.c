#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){

//początek jak u Pakera, nie chce mi się przepisywać


        while((file_dest=readdir(dest)) != NULL){

                found = false;
                rewinddir(src);
                while(file_src=readdir(src) != NULL){

                        if(strcmp(file_dest->d_name, file_src->d_name) == 0 && file_dest->d_type != DT_>                                found = true;
                                break;
                        }

                }

                if(found == false){
                        unlink(file_dest);
                }

        }

        closedir(src);
        closedir(dest);
        return 0;

}