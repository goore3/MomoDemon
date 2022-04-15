#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

int main(int argc, char* argv[])
{
	bool found;
	DIR *src, *dest;
	struct stat statbuf;
	struct stat statbuf2;
	char  src_path[100], dest_path[100];
	char s[] = "A/", d[] = "B/";
	src = opendir(s);
	dest = opendir(d);
	struct dirent *file, *file2;
	char date[36];
	while((file = readdir(src)) != NULL){
		found = false;
		//wraca wskaznik na poczatek foldera B
		rewinddir(dest);
		while((file2 = readdir(dest)) != NULL){
			//porownywanie plikow z folderu A i B
			if(strcmp(file->d_name, file2->d_name) == 0 && file->d_type != DT_DIR){
				found = true;
				//wypisywanie sciezek dla stat
				strcpy(src_path, s);
				strcat(src_path, file->d_name);
				strcpy(dest_path, d);
				strcat(dest_path, file2->d_name);
				stat(src_path, &statbuf);
				stat(dest_path, &statbuf2);
				//printf("src: %s\ndest: %s\n", src_path, dest_path);

				//porownanie czasu modyfikacji plikow
				if(statbuf.st_mtime > statbuf2.st_mtime){printf("Nadpisujemy plik %s w folderze B\n", file->d_name);}
				//printf("Data modyfikacji pliku %s w A: %ld\nData modyfikacji pliku %s w B: %ld\n", file->d_name, statbuf.st_mtime, file2->d_name, statbuf2.st_mtime);
				break;
			}
		}
		//kopiowanie pliku do folderu B(bedzie)
		if(!found && file->d_type != DT_DIR){printf("Kopiujemy plik %s do %s\n", file->d_name, d);}
	}

	closedir(src);
	closedir(dest);
	return 0;
}
