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

void copy(char *src_path, char *dest_path)
{
	char buf;
	int src_fd, dest_fd;

	src_fd = open(src_path, O_RDONLY);
	if(src_fd == -1){
		printf("Blad otwierania pliku src.\n");
		close(src_fd);
	}

	dest_fd = open(dest_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	while(read(src_fd, &buf, 1)){
		write(dest_fd, &buf, 1);
	}
	printf("LOG: kopia pliku %s udana.\n", src_path);
	close(src_fd);
	close(dest_fd);
}

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
			//wypisywanie sciezek dla stat
                        strcpy(src_path, s);
                        strcat(src_path, file->d_name);

			//porownywanie plikow z folderu A i B
			if(strcmp(file->d_name, file2->d_name) == 0 && file->d_type != DT_DIR){
				found = true;
				//wypisywanie sciezek dla stat
				strcpy(dest_path, d);
				strcat(dest_path, file2->d_name);
				stat(src_path, &statbuf);
				stat(dest_path, &statbuf2);
				//printf("src: %s\ndest: %s\n", src_path, dest_path);

				//porownanie czasu modyfikacji plikow
				if(statbuf.st_mtime > statbuf2.st_mtime)
				{
					printf("Nadpisujemy plik %s w folderze B\n", file->d_name);
					copy(src_path, dest_path);
				}
				printf("Data modyfikacji pliku %s w A: %ld\nData modyfikacji pliku %s w B: %ld\n", file->d_name, statbuf.st_mtime, file2->d_name, statbuf2.st_mtime);
				break;
			}
		}
		//kopiowanie pliku do folderu B(bedzie)
		if(!found && file->d_type != DT_DIR){
			//tworzenie sciezki dla nowego pliku w katalogu B
			strcpy(dest_path, d);
			strcat(dest_path, file->d_name);
			printf("Kopiujemy plik %s do %s\n", file->d_name, d);
			copy(src_path, dest_path);
		}
	}

	closedir(src);
	closedir(dest);
	return 0;
}
