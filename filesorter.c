#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<dirent.h>

struct arrayInfo{
	int length;
	unsigned int *numbs;
	int index;
};

struct circularBuffer{
	void **array;
	int front;
	int back;
	int count;
	int length;
	pthread_mutex_t lock;
	pthread_cond_t condition;
};

struct object{
	char *directory;
	int threadCount;
	struct circularBuffer Buff;
	struct circularBuffer Buff2;
	int start;
	int stop;
};

void inBuf(struct circularBuffer* B, int L)
{

	B->array=(void**)malloc(sizeof(void *)*L);
	B->back=0;
	B->front=0;
	B->count=0;
	B->length=L;
	pthread_mutex_init(&B->lock,NULL);
	pthread_cond_init(&B->condition,NULL);


}
void adder(struct circularBuffer * buf,void *data)
{
	pthread_mutex_lock(&buf->lock);
	while(buf->count==buf->length)
	{
		pthread_cond_wait(&buf->condition,&buf->lock);
	}
	buf->array[buf->back]=data;
	buf->back=(buf->back+1)%buf->length;
	buf->count++;

	pthread_cond_signal(&buf->condition);
	pthread_mutex_unlock(&buf->lock);
}
void* subber(struct circularBuffer * buf)
{
	pthread_mutex_lock(&buf->lock);
	while(buf->count==0)
	{
		pthread_cond_wait(&buf->condition,&buf->lock);
	}

	void * data=buf->array[buf->front];
	buf->front=(buf->front+1)%buf->length;
	buf->count--;

	pthread_cond_signal(&buf->condition);
	pthread_mutex_unlock(&buf->lock);

	return data;
}
void *reader(void *tidptr)
{
	int count=0;
	struct object *ONE=(struct object*)tidptr;


	DIR * openedDirectory =	opendir(ONE->directory);

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
		if(strncmp(Entry->d_name,"unsorted",strlen("unsorted"))==0)
		{
			count++;

			if(count<ONE->start || count>=ONE->stop){
				continue;
			}
			char path[1024];
			sprintf(path,"%s/%s",ONE->directory,Entry->d_name);

		//	FILE * fp = fopen(path,"r");
			int fp = open(path,O_RDONLY);
			struct stat buf;
			int Stat = fstat(fp,&buf);
	//		int Stat = stat(fp,&buf);
	//		
			if(fp<0){
				fprintf(stderr,"error with reader\n");
				close(fp);
				continue;
			}
			if(Stat<0)
			{

				fprintf(stderr,"error with reader\n");
				close(fp);
				continue;
			}
			unsigned int *numbers=(unsigned int*)malloc(sizeof(unsigned int)*buf.st_size/4);
			struct arrayInfo *Info=(struct arrayInfo*)malloc(sizeof(struct arrayInfo));
			Info->index=count;
			Info->length=buf.st_size/4;
			Info->numbs=numbers;

			read(fp,numbers,buf.st_size);
			adder(&ONE->Buff,Info);
			close(fp);


		}

	}	
	adder(&ONE->Buff,NULL);

	return NULL;
}

void *writer(void *tidptr)
{
	
	struct object *ONE=(struct object*)tidptr;
	while(1)
	{
		struct arrayInfo* Use=(struct arrayInfo*)subber(&ONE->Buff2);
//		printf("writter dequed: %p\n",Use);
		if(Use==NULL){
			break;
		}
		char Name[1024];
		//not sure if need, but will if directory not created in main
		sprintf(Name,"%s/sorted/sorted_%d.bin",ONE->directory,Use->index);

		int fp=open(Name,O_WRONLY|O_CREAT,0644);
		if(fp<0){
			fprintf(stderr,"%s :file could not open\n",Name);
			continue;
		}
		//printf("NAME CREATED IS: %s\n",Name); 
		for(int j = 0; j <Use->length;j++){
			//writes in here
			write(fp,&Use->numbs[j],4);

		}
	

		close(fp);
	}

	return NULL;
}
void swap(unsigned int *xp,unsigned int *yp)
{
	unsigned int temp = *xp;
	*xp=*yp;
	*yp=temp;
}
void mysort(struct arrayInfo* stuff){
	int L = stuff->length;
	int n = L;


	while(n>1)
	{
		int newn=0;

		for(int j = 1; j<=n-1;j++){
			if(stuff->numbs[j-1] > stuff->numbs[j])
			{
				swap(&stuff->numbs[j-1],&stuff->numbs[j]);
				newn=j;
			}
		}
		n=newn;

	}

}

void *sorter(void *tidptr)
{

	struct object *ONE=(struct object*)tidptr;

	while(1){
		struct arrayInfo *numbers=(struct arrayInfo*)subber(&ONE->Buff);
//		printf("sorter dequed %p\n",numbers); 

		if(numbers==NULL){
			break;
		}
		mysort(numbers);//should sort here
		//printf("sorter is adding to the que: %p\n",numbers);
		adder(&ONE->Buff2,numbers);	



	}


	adder(&ONE->Buff2,NULL);
	return NULL;

}
void testsort(){
	unsigned int array[10]={10,9,20,4,11,22,15,16,8,11};
	struct arrayInfo tester	;
	tester.numbs=array;
	tester.length=10;

	mysort(&tester);

	for(int j =0; j<tester.length;j++){
		printf("%u\n",tester.numbs[j]);

	}
}
int countUnsortedFiles(char * directory){
	int counter=0;

	DIR * openedDirectory =	opendir(directory);
	while(1)
	{

	
		struct dirent * Entry = readdir(openedDirectory);
		if(Entry ==NULL){
			break;
		}
		if(strncmp(Entry->d_name,"unsorted",strlen("unsorted"))==0)
		{
			counter++;
		}
		
	}
	closedir(openedDirectory);
	return counter;
}


int main(int argc,char *argv[])
{

//	testsort();

	if(argc != 3){
		fprintf(stderr,"there was an error with argument list\n");
		exit(-1);
	}

	char * location=argv[1];
	int setOfThreads =atoi(argv[2]);

	printf("location of directory: ");
	printf("%s\n",location);
	printf("number of set of threads: ");
	printf("%d\n",setOfThreads);
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
	int nFiles=countUnsortedFiles(argv[1]);	
	pthread_t t[setOfThreads*3];
	struct object tmp[setOfThreads];
	//might have error is exists
	//simalar to 29-252
	
	char path[1024];
	sprintf(path,"%s/sorted",argv[1]);
	mkdir(path,0755);


	//first i need to create threads to read files
	//do i close first then go to next set of threads of close all at once???
	for(int i = 0 ; i<setOfThreads; i++){
		//do stuff hear to create threads to read
		tmp[i].directory=argv[1];
		tmp[i].threadCount=setOfThreads;
		tmp[i].start=(nFiles/setOfThreads)*i;
		tmp[i].stop=tmp[i].start+(nFiles/setOfThreads);

		if(i==setOfThreads-1){
		tmp[i].stop=nFiles;
		}

		inBuf(&tmp[i].Buff,10);
		inBuf(&tmp[i].Buff2,10);

		
		rc=pthread_create(&t[i*3],NULL,reader,(void*)&tmp[i]);
		if(rc!=0){
			fprintf(stderr,"there was an error with reader\n");
			exit(-1);
		}
		
		rc=pthread_create(&t[i*3+1],NULL,sorter,(void*)&tmp[i]);
		if(rc!=0){
			fprintf(stderr,"there was an error with reader\n");
			exit(-1);
		}

		rc=pthread_create(&t[i*3+2],NULL,writer,(void*)&tmp[i]);
		if(rc!=0){
			fprintf(stderr,"there was an error with reader\n");
			exit(-1);
		}

	}

	for(int i=0;i<setOfThreads;i++){
		rc=pthread_join(t[i*3],NULL);
		if(rc!=0){
			fprintf(stderr,"error with joining\n");
		}
		rc=pthread_join(t[i*3+1],NULL);
		if(rc!=0){
			fprintf(stderr,"error with joining\n");
		}
		rc=pthread_join(t[i*3+2],NULL);
		if(rc!=0){
			fprintf(stderr,"error with joining\n");
		}
	}


}
