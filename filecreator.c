#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


struct object{
	char *directory;
	int threadID;
	int ET;
	int intsToInsert;
};




void *createFile(void *tidptr)
{
	struct object *ONE =(struct object *)tidptr;
	unsigned seed=ONE->threadID;

	for(int i = 0 ; i <ONE->ET ;i++)
	{
		char Name[1024];
		sprintf(Name,"%s/unsorted_%d.bin",ONE->directory,ONE->threadID*ONE->ET+i);
		//below is needed so if file doesnt exist it is created
		int fp = open(Name,O_WRONLY|O_CREAT,0644);
		//below i need to fill the file that was just created with rand integer data
		for(int j = 0; j<ONE->intsToInsert;j++)
		{
		int B=rand_r(&seed);
		write(fp,&B,4);
		}

		close(fp);

	}


	return NULL;
}



int main(int argc,char *argv[])
{

	if(argc != 5){
		fprintf(stderr,"there was an issue with argument list\n");
		exit(-1);
	}

	printf("location of directory: ");	
	printf("%s\n",argv[1]);
	printf("number of files: ");
	int F = atoi(argv[2]);
	printf("%d\n",F);	
	printf("number of integers to insert into each output file: ");	
	int R = atoi(argv[3]);
	printf("%d\n",R);
	printf("number of threads: ");
	int T = atoi(argv[4]);
	printf("%d\n",T);	
	int ET = F/T;
	
	printf("number of Files done by each thread is: ");
	printf("%d\n",ET);

	//	created threads below in loop
	int rc;
	pthread_t t[T];
	struct object tmp[T];

	struct stat buf;
	int Stat = stat(argv[1],&buf);

	//below needed to see if directory exists
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


	for(int i = 0; i<T; i++){
		//below is for what happens when files/threads does not divide cleanly
		if(i==T-1)
		{
		ET+=F-T*ET;
		}


		tmp[i].ET=ET;
		tmp[i].directory=argv[1];
		tmp[i].intsToInsert =R;
		tmp[i].threadID=i;

		rc = pthread_create(&t[i], NULL, createFile, (void *) &tmp[i]);
		if (rc){
			fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	for(int i = 0; i<T;i++){
		rc = pthread_join(t[i], NULL);

		if (rc != 0) {
			fprintf(stderr, "ERROR joining with thread w1. error==%d\n", rc);
			exit(-1);
		}
	}
}




