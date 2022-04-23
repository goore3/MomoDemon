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

bool f_recursive = true;

//funkcja odpowiedzialna za kopiowanie i nadpisywanie plikow z katalogu zrod³owego do katalogu docelowego
void copy(char *source_path, char *destination_path)
{
	char buf;
	int sourceFileDescriptor, destinationFileDescriptor;

	sourceFileDescriptor = open(source_path, O_RDONLY);
	if (sourceFileDescriptor == -1)
	{
		printf("Blad otwierania pliku src.\n");
		close(sourceFileDescriptor);
	}

	//wspominka: po dolaczeniu demona sprawdzic czy wlasciciel tworzonego pliku jest poprawny? 
	destinationFileDescriptor = open(destination_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	while (read(sourceFileDescriptor, &buf, 1))
	{
		write(destinationFileDescriptor, &buf, 1);
	}

	printf("LOG: kopia pliku %s udana.\n", source_path);
	close(sourceFileDescriptor);
	close(destinationFileDescriptor);
}

//funkcja odpowiedzialna za przeszukiwanie katalogow, kopiowanie plikow, tworzenie podkatalogow, usuwanie
void search(char *s, char *d)
{
	bool found_file, found_dir, first_round = true;
	DIR *src, *dest;
	struct stat statbuf;
	struct stat statbuf2;
	char source_path[100], destination_path[100], cmp_path[100];
	//printf("src: %s\ndest: %s\n", s, d);
	src = opendir(s);
	dest = opendir(d);
	struct dirent *file, *file2;

	while ((file = readdir(src)) != NULL)
	{
		found_file = false;
		found_dir = false;
		//wraca karetke na poczatek foldera B
		rewinddir(dest);
		while ((file2 = readdir(dest)) != NULL)
		{
			//wypisywanie sciezek dla stat
			strcpy(source_path, s);
			strcat(source_path, "/");
			strcat(source_path, file->d_name);
			//sprawdzanie czy podkatalog juz istnieje w katalogu docelowym
			if (strcmp(file->d_name, file2->d_name) == 0 && file->d_type == DT_DIR && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0 && f_recursive)
			{
				found_dir = true;
				printf("Folder %s\n", file->d_name);
				strcpy(destination_path, d);
				strcat(destination_path, "/");
				strcat(destination_path, file->d_name);
				search(source_path, destination_path);
			}

			//sprawdzanie istnienia pliku z katalogu docelowego w katalogu zrodlowym
			strcpy(cmp_path, s);
			strcat(cmp_path, "/");
			strcat(cmp_path, file2->d_name);
			if (first_round && file2->d_type == DT_REG && access(cmp_path, F_OK) == -1)
			{
				//usuwanie pliku nieistniejacego w katalogu zrodlowym
				printf("Plik %s nie istnieje w katalogu zrodlowym\n", file2->d_name);
			}

			//porownywanie plikow z katalogu zrodlowego i docelowego
			if (strcmp(file->d_name, file2->d_name) == 0 && file->d_type == DT_REG)
			{
				printf("Plik %s\n", file->d_name);
				found_file = true;
				//wypisywanie sciezek dla stat
				strcpy(destination_path, d);
				strcat(destination_path, "/");
				strcat(destination_path, file2->d_name);
				stat(source_path, &statbuf);
				stat(destination_path, &statbuf2);
				//printf("src: %s\ndest: %s\n", source_path, destination_path);

				//porownanie czasu modyfikacji plikow
				if (statbuf.st_mtime > statbuf2.st_mtime)
				{
					printf("Nadpisujemy plik %s w folderze %s\n", file->d_name, d);
					copy(source_path, destination_path);
				}

				//printf("Data modyfikacji pliku %s w %s: %ld\nData modyfikacji pliku %s w %s: %ld\n", file->d_name, s, statbuf.st_mtime, file2->d_name, d, statbuf2.st_mtime);
				break;
			}
		}

		first_round = false;

		if (!found_dir && file->d_type == DT_DIR && f_recursive && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
		{
			strcpy(destination_path, d);
			strcat(destination_path, "/");
			strcat(destination_path, file->d_name);
			printf("Tworzenie katalogu %s", file->d_name);
			mkdir(destination_path, 0755);
			search(source_path, destination_path);
		}

		//kopiowanie pliku do folderu B
		if (!found_file && file->d_type == DT_REG)
		{
			//tworzenie sciezki dla nowego pliku w katalogu B
			strcpy(destination_path, d);
			strcat(destination_path, "/");
			strcat(destination_path, file->d_name);
			printf("Kopiujemy plik %s do %s\n", file->d_name, d);
			copy(source_path, destination_path);
		}
	}

	closedir(src);
	closedir(dest);
}