#include <stdio.h>
#include<dirent.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

struct info{
	int numberOfThreads;
	char *directory;
	int start;
	int stop;
};

void *checker(void *tidptr)
{
	int count=0;
	struct info *one=(struct info*)tidptr;
	DIR *openedDirectory=opendir(one->directory);


	if(openedDirectory == NULL)
	{
		fprintf(stderr,"there was an error opening the directory given \n");
		exit(-1);
	}

	while(1)
	{


		struct dirent * Entry = readdir(openedDirectory);
		if(Entry ==NULL){
			break;
		}
		if(strncmp(Entry->d_name,"sorted",strlen("sorted"))==0)
		{
			count++;

			if(count<one->start || count>=one->stop){
				continue;
			}
			char path[1024];
			sprintf(path,"%s/%s",one->directory,Entry->d_name);

			int fp = open(path,O_RDONLY);
			struct stat oneStat;
			int resultOneStat= fstat(fp,&oneStat);
			if(fp<0){

				fprintf(stderr,"error with reader\n");
				close(fp);
				continue;
			}
			if(resultOneStat<0)
			{

				fprintf(stderr,"error with reader\n");
				close(fp);
				continue;
			}

			u_int32_t *mmapResult=mmap(NULL,oneStat.st_size,PROT_READ,MAP_PRIVATE,fp,0);

			//below is where i make sure the file is in order

			for(int i = 0; i<(oneStat.st_size/4)-1;i++){
				if(mmapResult[i]>mmapResult[i+1]){
					fprintf(stderr,"error THE NUMBERS ARE NOT IN ORDER\n");
					exit(-1);
				}
			}

			munmap(mmapResult,oneStat.st_size);

			close(fp);
		}
	}

	return NULL;

}

int countSortedFiles(char *directory){
	int counter=0;

	DIR * openedDirectory = opendir(directory);
	while(1)
	{


		struct dirent * Entry = readdir(openedDirectory);
		if(Entry ==NULL){
			break;
		}
		if(strncmp(Entry->d_name,"sorted",strlen("sorted"))==0)
		{
			counter++;
		}

	}
	closedir(openedDirectory);
	return counter;


}

int main(int argc,char *argv[])
{
	if(argc !=3){
		fprintf(stderr,"there was an issue with argument list\n");
		exit(-1);
	}

	char *unsortedDirectory = argv[1];
	int checkerThreads = atoi(argv[2]);

	printf("unsorted directory name: ");
	printf("%s\n",unsortedDirectory);
	printf("number of checker threads: ");
	printf("%d\n",checkerThreads);

	//below is needed to check to see if directory excists
	struct stat buf;
	int Stat = stat(argv[1],&buf);

	if(Stat<0)
	{
		fprintf(stderr,"ERROR directory does not exist\n");
		exit(-1);
	}
	if((buf.st_mode & S_IWUSR) != S_IWUSR)
	{

		fprintf(stderr,"ERROR directory is not writable\n");
		exit(-1);
	}

	int rc;
	pthread_t t[checkerThreads];
	char path[1024];
        sprintf(path,"%s/sorted",argv[1]);
	int nFiles=countSortedFiles(path);
	struct info tmp[checkerThreads];


	for(int i=0;i<checkerThreads;i++){
		tmp[i].directory=path;
		tmp[i].numberOfThreads=checkerThreads;
		tmp[i].start=(nFiles/checkerThreads)*i;
		tmp[i].stop=tmp[i].start+(nFiles/checkerThreads);


		rc=pthread_create(&t[i],NULL,checker,(void*)&tmp[i]);
	}
	for(int i =0;i<checkerThreads;i++){
		rc=pthread_join(t[i],NULL);
		if(rc!=0){
                        fprintf(stderr,"error with joining\n");
                }
	}







}

