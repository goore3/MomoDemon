#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char* argv[])
{
	bool found;
	DIR *src, *dest;
	struct stat statbuf;
	struct stat statbuf2;
	char  path[100], path2[100];
	char s[] = "A/", d[] = "B/";
	//chmod(s, S_IXOTH);
	//chmod(d, S_IXOTH);
	src = opendir(s);
	dest = opendir(d);
	strcpy(path, s);
	strcpy(path2, d);
	struct dirent *file, *file2;
	while((file = readdir(src)) != NULL){
		found = false;
		//wraca kolejke na sam poczatek folderu B
		rewinddir(dest);
		while((file2 = readdir(dest)) != NULL){
			//porownuje pliki z folderu A i B
			if(strcmp(file->d_name, file2->d_name) == 0 && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0){
				found = true;
				strcat(path, file->d_name);
				strcat(path2, file2->d_name);
				//printf("before tragedy");
				//chdir("~/A/");
				stat(path, &statbuf);
				//chdir("~/B/");
				stat(path2, &statbuf2);
				//chdir("~/A/");
				printf("Znaleziono plik %ld", statbuf.st_ino);
				//tu bedzie sprawdzanie daty modyfikacji za pomoca stat
				break;
			}
		}
		
		if(S_ISDIR(statbuf.st_mode) != 0){printf("Obiekt %s jest folderem\n", file->d_name);}
		//w razie nie znalezienia pliku w folderze B bedzie on kopiowany, tu wlasnie nie dziala S_ISDIR(pomocy)
		if(!found && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0 && S_ISDIR(statbuf.st_mode) == 0){printf("Kopiujemy plik %s do %s\n", file->d_name, d);}
	}

	closedir(src);
	closedir(dest);
	return 0;
}
