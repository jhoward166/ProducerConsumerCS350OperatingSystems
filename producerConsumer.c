#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#define DEFAULT_NUM_ASSIGNINGS        10
#define DEFAULT_MIN_PROF_WAIT         1
#define DEFAULT_MAX_PROF_WAIT         4
#define DEFAULT_MIN_NUM_ASSIGNMENTS   1
#define DEFAULT_MAX_NUM_ASSIGNMENTS   5
#define DEFAULT_MIN_ASSIGNMENT_HOURS  1 
#define DEFAULT_MAX_ASSIGNMENT_HOURS  5
#define DEFAULT_NUM_PROFESSORS        1 
#define DEFAULT_NUM_STUDENTS          5
#define DEFAULT_QUEUE_SIZE            5

int assign = DEFAULT_NUM_ASSIGNINGS;
int minWait = DEFAULT_MIN_PROF_WAIT;
int maxWait = DEFAULT_MAX_PROF_WAIT; 
int minAssign = DEFAULT_MIN_NUM_ASSIGNMENTS;
int maxAssign = DEFAULT_MAX_NUM_ASSIGNMENTS;
int minHours = DEFAULT_MIN_ASSIGNMENT_HOURS; 
int maxHours = DEFAULT_MAX_ASSIGNMENT_HOURS;
int prof = DEFAULT_NUM_PROFESSORS;
int stud = DEFAULT_NUM_STUDENTS;
int size = DEFAULT_QUEUE_SIZE;
int completed = 0;
int created = 0;
int inQueue = 0;
int inCrit = 0;
int live = 0;
int a, w, W, n, N, h, H, p, s, q, e = 0;
void * produce(int *);
void * consume(int *);

struct assignment{
	int hours;
	int num;
	int givenBy;
};
struct assignment * queue;

int flag = 1;
int flag2 = 1;
sem_t sShieldQ;
sem_t pShieldQ;
pthread_mutex_t protect_the_flag; 
pthread_cond_t condition_variable;

void paramCheck(char *param, char *val){
	if(strcmp(param,"-a")==0){
		a = 1;
		assign = atoi(val);
	}else if(strcmp(param,"-w")==0){
		w = 1;
		minWait = atoi(val);
	}else if(strcmp(param,"-W")==0){
		W = 1;
		maxWait = atoi(val);
	}else if(strcmp(param,"-n")==0){
		n = 1;
		minAssign = atoi(val);
	}else if(strcmp(param,"-N")==0){
		N = 1;
		maxAssign = atoi(val);
	}else if(strcmp(param,"-h")==0){
		h = 1;
		minHours = atoi(val);
	}else if(strcmp(param,"-H")==0){
		H = 1;
		maxHours = atoi(val);
	}else if(strcmp(param,"-p")==0){
		p = 1;
		prof = atoi(val);
	}else if(strcmp(param,"-s")==0){
		s = 1;
		stud = atoi(val);
	}else if(strcmp(param,"-q")==0){
		q = 1;
		size = atoi(val);
	}else{
		e = 1;
	}
}

void sanityCheck(){
	fprintf(stdout, "The following parameters were not set in the command line.\n");
	fprintf(stdout, "They will be set to their default values.\n");
	if(a == 0){
		fprintf(stdout, "Number of Assignments -a flag.\n");
	}
	if(w == 0){
		fprintf(stdout, "Minimum Prof. Wait -w flag.\n");
	}
	if(W == 0){
		fprintf(stdout, "Maximum Prof. Wait -W flag.\n");
	}
	if(n == 0){
		fprintf(stdout, "Minimum Assignments -n flag.\n");
	}
	if(N == 0){
		fprintf(stdout, "Maximum Assignments -N flag.\n");
	}
	if(h == 0){
		fprintf(stdout, "Min Assignment Hours -h flag.\n");
	}
	if(H == 0){
		fprintf(stdout, "Max Assignment Hours -H flag.\n");
	}
	if(p == 0){
		fprintf(stdout, "Number of Professors -p flag.\n");
	}
	if(s == 0){
		fprintf(stdout, "Number of Students -s flag.\n");
	}
	if(q == 0){
		fprintf(stdout, "Queue Size -q flag.\n");
	}
}

void progParam(){
   	fprintf(stdout, "\nThe Program will run with the following parameters:\n");
   	fprintf(stdout, "Number of Assignments = %2d\n", assign);
   	fprintf(stdout, "   Minimum Prof. Wait = %2d\n", minWait);
   	fprintf(stdout, "   Maximum Prof. Wait = %2d\n", maxWait); 
   	fprintf(stdout, "  Minimum Assignments = %2d\n", minAssign);
   	fprintf(stdout, "  Maximum Assignments = %2d\n", maxAssign);
   	fprintf(stdout, " Min Assignment Hours = %2d\n", minHours); 
   	fprintf(stdout, " Max Assignment Hours = %2d\n", maxHours);
   	fprintf(stdout, " Number of Professors = %2d\n", prof);
   	fprintf(stdout, "   Number of Students = %2d\n\n", stud);
   	fprintf(stdout, "           Queue Size = %2d\n\n", size);
}

int main(int argc, char ** argv){
	int i = 1;
	int j=0;
	for(i; i< argc; i+=2){
		paramCheck(argv[i], argv[i+1]);
	}
	if(e==1){
		printf("Error: Parameter not recognized.\n");
		return 0;
	}
	sanityCheck();
	progParam();
	queue = malloc(sizeof(struct assignment)*size);
	pthread_mutex_init(&protect_the_flag, NULL);
    pthread_cond_init(&condition_variable, NULL);
	sem_init(&pShieldQ, 0, size);
	sem_init(&sShieldQ, 0, 0);
	pthread_t professor[prof];
	for(j=0; j< prof; j++){		
		int profID[prof];
		profID[j] = j+1;
		if(pthread_create(&professor[j], NULL, (void * (*)(void *)) produce, &(profID[j]))!=0){
			printf("Thread could not be created.\n");
			return -1;
		}else{
			live++;
		}
	}
	pthread_t student[stud];
	int * studID = malloc(sizeof(int)*stud);
	for(i=0; i< stud; i++){		
		studID[i] = i+1;
		//printf("%d\n", studID);
		if(pthread_create(&student[i], NULL, (void * (*)(void *))consume, &(studID[i]))!=0){
			printf("Thread could not be created.\n");
			return -1;
		}
	}
	for(j=0; j< prof; j++){
		if(pthread_join(professor[j], NULL) != 0){
			printf("pthread_join\n");
			return -1;
		}
	}
	for(i=0; i<stud; i++){
		if(pthread_join(student[i], NULL) != 0){
			printf("pthread_join\n");
			return -1;
		}
	}
	free(studID);
	free(queue);
	return 0;
}


void * produce(int *passID){
	int b = 0;
	int z = 0;
	int assigned = 0;
	int quantAssign;
	int profID = *passID;
	//fprintf(stdout,"\n\n\n%d\n\n\n\n", profID);
	int diff = maxWait-minWait;
	srand(time(NULL));
	int waitTime = ((rand()%diff)+1)+minWait;
	sleep(waitTime);
	srand(time(NULL));
	diff = maxAssign - minAssign;
	quantAssign = ((rand()%diff)+1)+minAssign;
	for(b; b<quantAssign;){

		//NEEDS TO BLOCK IF QUEUE FULL
		sem_wait(&pShieldQ);

		diff = maxHours - minHours;
		srand(time(NULL));
		struct assignment hw;
		hw.hours = ((rand()%diff)+1)+minHours;
		hw.num = b+1;
		hw.givenBy = profID;
		//DONE~~~NEEDS TO BLOCK IF THERES SOMETHING IN CRIT
		pthread_mutex_lock(&protect_the_flag);
		while(flag == 0)
			pthread_cond_wait(&condition_variable, &protect_the_flag);
		flag = 0;
		pthread_mutex_unlock(&protect_the_flag);
		if(assigned < quantAssign){
			inCrit++;
			queue[inQueue]=hw;
			created++;
			inQueue++;
			assigned++;
			if(b== (quantAssign-1)){
				live--;
			}
		}
		inCrit--;
			//UNBLOCK
		pthread_mutex_lock(&protect_the_flag);
		flag = 1;
		pthread_mutex_unlock(&protect_the_flag); 
		pthread_cond_signal(&condition_variable);
		 
		sem_post(&sShieldQ);
		b++;
		sleep(1);
	}
	printf("EXITING Professor %d\n", profID);
	pthread_exit(0);
	perror("produce");
}

void * consume(int* passID){
	int l = 0;
	int y = 0;
	int studID = *passID;
	int timeOut;
	int currentNum;
	int giver;
	struct assignment current;
	fprintf(stdout, "BEGIN Student %d \n", studID);
	while(live >0 || inQueue != 0){
		
		sem_wait(&sShieldQ);
		
		//DONE~~~NEEDS TO BLOCK IF QUEUE EMPTY

		//DONE~~~NEEDS TO BLOCK IF THERES SOMETHING IN CRIT
		pthread_mutex_lock(&protect_the_flag);
		while(flag == 0){
			pthread_cond_wait(&condition_variable, &protect_the_flag);
		}
		flag = 0;
		pthread_mutex_unlock(&protect_the_flag);
		if(inQueue > 0){
			inCrit++;
			current = queue[0];
			currentNum = current.num;
			timeOut = current.hours;
			giver = current.givenBy;
			for(y=0; y<inQueue; y++){
				queue[y]=queue[y+1];
			}
			inQueue--;
			fprintf(stdout, "Student %d working on Assignment %d from Professor %d.\n", studID, currentNum, giver);
			sleep(timeOut);
			completed++;
		}
		inCrit--;
		//UNBLOCK
		//printf("released %d \n", studID);
		pthread_mutex_lock(&protect_the_flag);
		flag = 1;
		pthread_mutex_unlock(&protect_the_flag); 
		pthread_cond_signal(&condition_variable);

		sem_post(&pShieldQ);
	}
	sem_post(&sShieldQ);
	fprintf(stdout, "EXITING Student %d\n", studID);
	pthread_exit(0);
	perror("consume");
}
