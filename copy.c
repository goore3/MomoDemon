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
#include <syslog.h>
#include <errno.h>
#include <sys/mman.h>

//funkcja odpowiedzialna za usuwanie niepustych folderow
void delDir(char *destination_path)
{
	char tmp_path[100];
	struct dirent *file;
	DIR *dest = opendir(destination_path);
	while((file = readdir(dest)) != NULL)
	{
		//zabezpieczenie aby nie bralo katalogow "." i ".." w budowanie sciezek
		if(strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
		{
			strcpy(tmp_path, destination_path);
			strcat(tmp_path, "/");
			strcat(tmp_path, file->d_name);
		}
		//rekurencyjne usuwanie zawartosci podkatalogu
		if(file->d_type == DT_DIR && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
		{
			delDir(tmp_path);
			remove(tmp_path);
			syslog(LOG_NOTICE, "Katalog %s zostal usuniety.\n", tmp_path);
		}
		//usuwanie pozostalych danych ktore nie sa katalogami
		if(file->d_type != DT_DIR)
		{
			remove(tmp_path);
			syslog(LOG_NOTICE, "Plik %s zostal usuniety.\n", tmp_path);
		}
	}
	closedir(dest);
}

//funkcja odpowiedzialna za kopiowanie i nadpisywanie plikow z katalogu zrodlowego do katalogu docelowego
void copy(char *source_path, char *destination_path, long int mmapMinSize)
{
	int sourceFileDescriptor, destinationFileDescriptor;
	struct stat sb;
	sourceFileDescriptor = open(source_path, O_RDWR);
	if (sourceFileDescriptor == -1)
	{
		syslog(LOG_NOTICE, "Blad otwierania pliku %s: %s\n", source_path, strerror(errno));
		close(sourceFileDescriptor);
	}

	int err = stat(source_path, &sb);
	if(err < 0)
		syslog(LOG_NOTICE, "Blad otwierania informacji stat pliku %s: %s\n", source_path, strerror(errno));

	destinationFileDescriptor = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(sb.st_size < mmapMinSize)
	{
		char buf[1024];
		syslog(LOG_NOTICE, "Kopiowanie pliku %s read/write\n", source_path);
		ssize_t mem = read(sourceFileDescriptor, buf, 1024);
		while (mem > 0)
		{
			write(destinationFileDescriptor, buf, mem);
			mem = read(sourceFileDescriptor, buf, 1024);
		}
		
	}
	else
	{
		syslog(LOG_NOTICE, "Kopiowanie pliku %s mmap/write\n", source_path);
		char *ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, sourceFileDescriptor, 0);
		if(ptr == MAP_FAILED) 
			syslog(LOG_NOTICE, "Mapowanie sie niepowiodlo dla pliku %s!!!: %s\n", source_path, strerror(errno));

		ssize_t n = write(destinationFileDescriptor, ptr, sb.st_size);
		if(n != sb.st_size) 
			syslog(LOG_NOTICE, "Write failed");

		err = munmap(ptr, sb.st_size);

		if(err != 0)
			syslog(LOG_NOTICE, "Unmapowanie sie niepowiodlo\n");
    
	}
	syslog(LOG_NOTICE, "Kopia pliku %s udana.\n", source_path);
	close(sourceFileDescriptor);
	close(destinationFileDescriptor);
}

//funkcja odpowiedzialna za przeszukiwanie katalogow, kopiowanie plikow, tworzenie podkatalogow, usuwanie
void synchronization(char *s, char *d, int f_recursive, long int mmapMinSize)
{
	//syslog(LOG_NOTICE, "synchronizacja dla %s\n", s);
	bool found_file, found_dir, first_round = true;
	DIR *src, *dest;
	struct stat statbuf;
	struct stat statbuf2;
	char source_path[100], destination_path[100], cmp_path[100];
	//otwieranie katalogow
	src = opendir(s);
	dest = opendir(d);
	struct dirent *file, *file2;

	while ((file = readdir(src)) != NULL)
	{
		//flagi informujace o znalezieniu pliku lub podkatalogu z katalogu zrodlowego w katalogu docelowym
		found_file = false;
		found_dir = false;
		//wypisywanie sciezek dla plikow z katalogu zrodlowego
		strcpy(source_path, s);
		strcat(source_path, "/");
		strcat(source_path, file->d_name);
		//wraca karetke na poczatek katalogu docelowego
		rewinddir(dest);
		while ((file2 = readdir(dest)) != NULL)
		{
			
			//sprawdzanie czy podkatalog juz istnieje w katalogu docelowym
			if (strcmp(file->d_name, file2->d_name) == 0 && file->d_type == DT_DIR && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0 && f_recursive)
			{
				found_dir = true;
				//syslog(LOG_NOTICE, "Folder %s\n", file->d_name);
				strcpy(destination_path, d);
				strcat(destination_path, "/");
				strcat(destination_path, file->d_name);
				synchronization(source_path, destination_path, f_recursive, mmapMinSize);
			}

			//tworzenie sciezki dla plikow z katalogu docelowego z naniesieniem na katalog zrodlowy w celu znalezienia nie istniejacych tam plikow
			strcpy(cmp_path, s);
			strcat(cmp_path, "/");
			strcat(cmp_path, file2->d_name);
			//sprawdzanie istnienia pliku lub katalogu z katalogu docelowego w katalogu zrodlowym
			if (first_round && (file2->d_type == DT_REG || file2->d_type == DT_DIR) && access(cmp_path, F_OK) == -1 && strcmp(file2->d_name, ".") != 0 && strcmp(file2->d_name, "..") != 0)
			{
				strcpy(destination_path, d);
				strcat(destination_path, "/");
				strcat(destination_path, file2->d_name);
				//rekurencyjne wchodzenie do katalogow w celu ich wyczyszczenia
				if(f_recursive && file2->d_type == DT_DIR)
				{
					//syslog(LOG_NOTICE, "rekurencja katalogu. %s\n", destination_path);
					delDir(destination_path);
					remove(destination_path);
					syslog(LOG_NOTICE, "Katalog %s zostal usuniety.\n", destination_path);
				}

				//usuwanie z katalogu docelowego pliku nieistniejacego w katalogu zrodlowym
				if(file2->d_type == DT_REG)
				{
					remove(destination_path);
					syslog(LOG_NOTICE, "Plik %s zostal usuniety.\n", destination_path);	
				}
			}

			//porownywanie plikow z katalogu zrodlowego i docelowego
			if (strcmp(file->d_name, file2->d_name) == 0 && file->d_type == DT_REG)
			{
				found_file = true;
				//wypisywanie sciezek dla stat
				strcpy(destination_path, d);
				strcat(destination_path, "/");
				strcat(destination_path, file2->d_name);
				stat(source_path, &statbuf);
				stat(destination_path, &statbuf2);

				//porownanie czasu modyfikacji plikow
				if (statbuf.st_mtime > statbuf2.st_mtime)
				{
					syslog(LOG_NOTICE, "Nadpisywanie pliku %s w folderze %s\n", file->d_name, d);
					copy(source_path, destination_path, mmapMinSize);
				}

				break;
			}
		}
		//flaga sygnalizujaca czy jest to pierwsze przejscie readdira przez katalog docelowy
		first_round = false;

		//jezeli nie znaleziono katalogu w docelowym katalogu to jest on tworzony na nowo
		if (!found_dir && file->d_type == DT_DIR && f_recursive && strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
		{
			strcpy(destination_path, d);
			strcat(destination_path, "/");
			strcat(destination_path, file->d_name);
			syslog(LOG_NOTICE, "Tworzenie katalogu %s", file->d_name);
			mkdir(destination_path, 0755);
			synchronization(source_path, destination_path, f_recursive, mmapMinSize);
		}

		//tworzenie i kopiowanie pliku do katalogu docelowego jezeli go tam nie bylo
		if (!found_file && file->d_type == DT_REG)
		{
			//tworzenie sciezki dla nowego pliku w katalogu docelowym
			strcpy(destination_path, d);
			strcat(destination_path, "/");
			strcat(destination_path, file->d_name);
			syslog(LOG_NOTICE, "Tworzenie pliku %s do %s\n", file->d_name, d);
			copy(source_path, destination_path, mmapMinSize);
		}
	}
	//jezeli w katalogu zrodlowym istnieje pusty podkatalog, a w katalogu docelowym istnieje ten sam podkatalog z zawartoscia to jest ona usuwana z niego
	if(file == NULL && first_round && f_recursive)
	{
		while((file2 = readdir(dest)) != NULL)
		{	
			strcpy(destination_path, d);
			strcat(destination_path, "/");
			strcat(destination_path, file2->d_name);
			if(file2->d_type == DT_DIR)
			{
				//rekurencyjne wchodzenie do podkatalogow w katalogu docelowym w celu usuniecia jego zawartosci
				delDir(destination_path);
				remove(destination_path);
				syslog(LOG_NOTICE, "Katalog %s zostal usuniety.\n", destination_path);
			}
			else
			{
				//usuwanie plikow ktore nie istnieja w katalogu zrodlowym
				remove(destination_path);
				syslog(LOG_NOTICE, "Plik %s zostal usuniety.\n", destination_path);
			}
		}
	}

	closedir(src);
	closedir(dest);
}
