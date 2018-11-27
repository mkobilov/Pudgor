#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>

#define MAXLENGTH 128		//Maximum length of name of a file or dir

struct keys
{ 
	int l_ind;
	int n_ind;
	int a_ind;
	int R_ind;
	int d_ind;
	int i_ind;
	
};

int DirNum(char* dir_name)				//Counting the number of files and dirs in directory we are in
{
	int file_count = 0;
	DIR * dir;
	struct dirent* myfile;

	dir = opendir(dir_name); 			
	if(dir == NULL) {
		fprintf(stderr,"opendir ERR occured: %s\n", strerror(errno));
		return 0;
	}
	while (((myfile = readdir(dir)) != NULL)) {
		file_count++;
	}
	closedir(dir);
	
	return file_count;
}

char* FormName(char* name1, char* name2)			
{
	char* buf = (char*) calloc (strlen(name1) + strlen(name2) + 2, sizeof(char));
	strcat(buf, name1);	
	strcat(buf, "/");	
	strcat(buf, name2);	
	return buf;
}

void PrintInfo(char* dir_name, char* myfile, DIR* mydir, struct keys key)
{
	struct stat mystat;	
	char buf[1024];
	
	if(key.d_ind != 1)
		sprintf(buf, "%s/%s", dir_name, myfile);
	else
		sprintf(buf, "%s", myfile);
	lstat(buf, &mystat);
		
	if(key.i_ind == 1) {
		printf("%ld\t", mystat.st_ino);
	}
	if(key.l_ind == 1 || key.n_ind == 1) {
		//MODE
		
		
		if(S_ISREG(mystat.st_mode))
			printf("-");
		else {
			if(S_ISDIR(mystat.st_mode)) {
				printf("d");
			}
			if(S_ISLNK(mystat.st_mode)) {
				printf("l");
			}
		}
		printf((mystat.st_mode & S_IRUSR) ? "r" : "-");
		printf((mystat.st_mode & S_IWUSR) ? "w" : "-");
		printf((mystat.st_mode & S_IXUSR) ? "x" : "-");
		printf((mystat.st_mode & S_IRGRP) ? "r" : "-");
		printf((mystat.st_mode & S_IWGRP) ? "w" : "-");
		printf((mystat.st_mode & S_IXGRP) ? "x" : "-");
		printf((mystat.st_mode & S_IROTH) ? "r" : "-");
		printf((mystat.st_mode & S_IWOTH) ? "w" : "-");
		printf((mystat.st_mode & S_IXOTH) ? "x" : "-");
		printf("\t");
		
		//NUMBER OF LINKS
		
		printf("%ld\t",mystat.st_nlink);
		
		//USER AND GROUP INFO
		
		if(key.n_ind == 0){				//usr and gr id, shown as numbers if -n key is used or as words if -l key is used
			struct group *grp;
			struct passwd *pwd;

			errno = 0;
			
			pwd = getpwuid(mystat.st_uid);
			if(errno != 0)
				fprintf(stderr,"getpwuid ERR occured: %s\n", strerror(errno));
			if(pwd != NULL)
				printf("%s\t", pwd->pw_name);
			else
				printf("%d\t", mystat.st_uid);
				
			grp = getgrgid(mystat.st_gid);
			if(grp != NULL)
				printf("%s\t", grp->gr_name);
			else
				printf("%d\t", mystat.st_gid);
		}
		else{
			printf("%d\t", mystat.st_uid);
			
			printf("%d\t", mystat.st_gid);
		}
		//SIZE
		
		printf("%ld\t",mystat.st_size);
		
		//TIME OF LAST MODIFICATION
		
		char date[50];
		time_t now = time(0);
		
		if(((localtime(&(mystat.st_ctime)))->tm_year) != (localtime(&now)->tm_year)) {
			strftime(date, 50, "%b\t%d\t%Y\t", localtime(&(mystat.st_ctime)));
			printf("%s", date);
		}
		else {
			strftime(date, 50, "%b\t%d\t%H:%M\t", localtime(&(mystat.st_ctime)));
			printf("%s", date);
		}
	}
	
	if(S_ISLNK(mystat.st_mode) && (key.l_ind  || key.n_ind)) {
		int charnumber = readlink(myfile, buf, MAXLENGTH);		//Reading link to buf and putting \0 at the end of buf
		buf[charnumber] = '\0';									//because readlink does not do it, so printf will not work	
		printf("%s -> %s\n", myfile, buf);						//properly otherwise.
	}
	else {
		printf("%s\n", myfile);
	}
}

void PrintDir(char* dir_name, struct keys key) 
{
	DIR* mydir;
	struct dirent* myfile;
	
	int i = 0;						
	int dn = DirNum(dir_name);
	char** dirlist = (char**) calloc (dn, sizeof(char*));
	while(i < dn) {
		dirlist[i] = (char*) calloc(MAXLENGTH, sizeof(char));
		i++;
	}
	i = 0;

	mydir = opendir(dir_name);
	if(mydir == NULL) {
		fprintf(stderr,"opendir ERR occured: %s\n", strerror(errno));
		return ;
	}
	
	
	if(key.d_ind != 1)
		while((myfile = readdir(mydir)) != NULL) {
			
			if(key.a_ind == 1) {
				
				PrintInfo(dir_name, myfile->d_name, mydir, key);
				
				if(key.R_ind == 1) {				//Adding dirs to list if -R key is used
					struct stat mystat;	
					char buf[1024];
					sprintf(buf, "%s/%s", dir_name, myfile->d_name);
					stat(buf, &mystat);
					if(S_ISDIR(mystat.st_mode) && strcmp(myfile->d_name, ".") && strcmp(myfile->d_name, "..")) {		
						strcpy(dirlist[i], myfile->d_name);
						i++;
					}
				}
			}
			else {
				if((myfile->d_name)[0] != '.') {
					
					PrintInfo(dir_name, myfile->d_name, mydir, key);
					
					if(key.R_ind == 1) {
						struct stat mystat;	
						char buf[1024];
						sprintf(buf, "%s/%s", dir_name, myfile->d_name);
						stat(buf, &mystat);
						if(S_ISDIR(mystat.st_mode) && strcmp(myfile->d_name, ".") && strcmp(myfile->d_name, "..")) {
							strcpy(dirlist[i], myfile->d_name);
							i++;
						}
					}
				}
			}
		}	
	else{
		PrintInfo(NULL, dir_name, NULL, key);
	}
	closedir(mydir);
	printf("\n");
	
	if(key.R_ind == 1){		//Calling PrintDir if -R key is used
		int k = 0;
		while(k < i){
			printf("%s\n",dirlist[k]);
			PrintDir(FormName(dir_name, dirlist[k]), key);
			//printf("  %s  ",dirlist[k]);
			k++;
		}
		
	}
	
	//MEMORY FREE
	i = 0;			
	while(i < dn) {
		free(dirlist[i]);
		i++;
	}
	free(dirlist);
	free(dir_name);
	
	printf("\n");
}

	


int main(int argc, char* argv[])

{
	//Flags
	struct keys key = {0, 0, 0, 0, 0};
	struct option longopts[] = {
		{"long", 0, NULL, 'l'},
		{"inode", 0, NULL, 'i'},
		{"recursive", 0, NULL, 'R'},
		{"numeric-id-uid", 0, NULL, 'n'},
		{"directory", 0, NULL, 'd'},
		{"all", 0, NULL, 'a'}
		
	};
	int ind = getopt_long(argc, argv, "ilnaRd", longopts, NULL);
	
	while(ind != -1) {
		switch(ind) {
			case('l'): {key.l_ind = 1; break;}
			case('n'): {key.n_ind = 1; break;}
			case('a'): {key.a_ind = 1; break;}
			case('R'): {key.R_ind = 1; break;}
			case('i'): {key.i_ind = 1; break;}
			case('d'): {key.d_ind = 1; break;}
			default  :  {break;}
		}
		
		ind = getopt_long(argc, argv, "lnaiR", longopts, NULL);
		
	}
	
	char* dirname = (char*) calloc (MAXLENGTH,sizeof(char));
	
	if(argv[optind] == NULL) 
		strcpy(dirname,".");
	
	else
		strcpy(dirname,argv[optind]);
	PrintDir(dirname, key);
	return 0;
}
