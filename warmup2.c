#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include "cs402.h"
#include "my402list.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

typedef struct tagMy402Packet {
	long int packet_num;	
	double time_packet_arrival;
	double Start_Time_Q1;
	double time_end_from_Q1;	
	double Start_Time_Q2;
	double time_end_from_Q2_S1;
	double time_end_from_Q2_S2;	
	double time_start_in_S1;
	double time_end_from_S1;
	double time_start_in_S2;
	double time_end_from_S2;		
    int inter_arrival_time;
    int service_time;
    int tokens_need;
} My402Packet;

int errno ;
char emulation_mode;
int flag = 0;
const char s[4] = " \t\n"; 
char* tok;
int check_status=0;
char line[1024];
char line_check[1024];
My402Packet * packet;
int Server_Interupt = 0;

int k=-1;
FILE * fp=NULL;
FILE * fp_check=NULL;

double lambda, mu,r,B,P;
long int n;
int number_of_packets;
My402List Q1, Q2, Final_List, Collect_List;

pthread_mutex_t Mutex;
pthread_cond_t Queue_Not_Empty;
sigset_t Signal_Mask;
struct timeval Start_Time;
pthread_t packet_id, token_id, server_id_1,server_id_2,interupt_id;
long int Packet_ID,Discard_Packet,temp_Bucket,Server_Guard, Token_ID, Discard_Token = 0;
void CreateTestList(My402List*, My402Packet *);

double timer(struct timeval End_Time)
{
	double real_part = 1000 * (End_Time.tv_sec - Start_Time.tv_sec);//this is in seconds if you want to express in ms * 1000
	double decimal_part = ((double)(End_Time.tv_usec - Start_Time.tv_usec)) / 1000;//this is already in us if you want to express in ms divide by 1000
	double time_micro_sec = real_part + decimal_part;
	return time_micro_sec;
}

void CreateTestList(My402List *pList,My402Packet* ptr)
{
        (void)My402ListAppend(pList, ptr);
    
}

void packet_handler(int sig)
{
    pthread_mutex_trylock(&Mutex);
    My402ListUnlinkAll(&Final_List);
    My402ListUnlinkAll(&Q1);
    pthread_mutex_unlock(&Mutex);
    pthread_exit(0);
}

void token_handler(int sig)
{
    pthread_mutex_trylock(&Mutex);
    My402ListUnlinkAll(&Q2);
    Server_Guard = 0;
    pthread_mutex_unlock(&Mutex);
    pthread_exit(0);
}

int prepare_dpacket(){
        double interval_packet;
        double interval_tokens;
        My402Packet * packet;
        packet = (My402Packet *)malloc(sizeof(My402Packet));
        interval_packet = (1 / lambda) * 1000000;
        if(interval_packet <= 0 || interval_packet >= (10 * 1000000)){
        interval_packet = 10 * 1000000;
        }
        packet -> inter_arrival_time = interval_packet;
        packet -> tokens_need = P;
        interval_tokens = (1 / mu) * 1000000;
        if(interval_tokens <= 0 || interval_tokens >= (10 * 1000000)){
        interval_tokens = 10 * 1000000;
        }else{
        interval_tokens=interval_tokens;
        }       
        packet -> service_time = interval_tokens;
        CreateTestList(&Final_List,packet);
        return(0);  
    }

int call(){

	if(emulation_mode=='t'){
	int i;

    if (fgets(line,sizeof(line), fp) != NULL) {
    tok = strtok(line, s);
    if(k==-1){ 
    	number_of_packets = atoi(tok);
    	if(number_of_packets == 0){
    		printf("malformed input - line 1 is not just a number\n");
    		exit(0);
    	}    	
    	k=0;
    	return(0);
        }        	
	    if(strlen(line)>1024){
        fprintf(stderr,"Line exceeds 1024 characters\n");
            exit(1);
    	} 
    	if(strlen(line)==0){
        fprintf(stderr,"Line is empty. Please check the file.\n");
            exit(1);
    	}
    
    packet = (My402Packet *)malloc(sizeof(My402Packet));
    i=0;


    while (tok != 0) { 
   	
        if(i==0){
        	packet->inter_arrival_time=(int)malloc (strlen(tok)*sizeof(My402Packet));
			packet->inter_arrival_time=atoi(tok)*1000;
			}

        else if(i==1){
        	packet->tokens_need=(int)malloc (strlen(tok)*sizeof(My402Packet));
        	packet->tokens_need=atoi(tok);
        }
            
        else if(i==2){
        	packet->service_time=(int)malloc (strlen(tok)*sizeof(My402Packet));
        	packet->service_time=atoi(tok)*1000;
        }

    tok = strtok(0, s);
    i++;
    } 
    CreateTestList(&Final_List,packet);
		}
	}
	return(0);
	}
		
	
void packet_statistics()
{
	double Final_Packet;
	if(My402ListEmpty(&Q2))
	{
		My402ListUnlinkAll(&Q2);
 		Server_Guard = 0;
	}
	flag = flag + 1;
	pthread_mutex_unlock(&Mutex);
	
	My402ListElem * elem;
	My402Packet * packet;
	struct timeval curr_time;
	double total_emulation_time = 0, all_time_spent_in_Q1 = 0, all_time_spent_in_Q2 = 0, all_time_spent_in_S1 = 0, all_time_spent_in_S2 = 0, 
	all_arrival_time = 0, packet_last_arrival_time = 0, all_server_time = 0, all_system_time = 0, deviation = 0, time_diff=0;
	long int served_packet_num = 0;
	gettimeofday(&curr_time, 0);
	total_emulation_time = timer(curr_time);
	elem = My402ListFirst(&Collect_List);

	if(flag == 2){	
		while(elem)
	{
		packet = (My402Packet *)(elem -> obj);

		all_arrival_time += (packet -> time_packet_arrival - packet_last_arrival_time);
		packet_last_arrival_time = packet -> time_packet_arrival;
		all_server_time += fabs(packet -> time_end_from_S1 - packet -> time_start_in_S1) + fabs(packet -> time_end_from_S2 - packet -> time_start_in_S2);
		all_time_spent_in_Q1 += (packet -> time_end_from_Q1 - packet -> Start_Time_Q1);
		if(packet -> time_end_from_Q2_S1 ==0 || packet -> time_end_from_Q2_S1 == 0){
			all_time_spent_in_Q2 += fabs(packet -> time_end_from_Q2_S2 - packet -> Start_Time_Q2); 
		}else{
			all_time_spent_in_Q2 += fabs(packet -> time_end_from_Q2_S1 - packet -> Start_Time_Q2); 
		}
		all_time_spent_in_S1 += fabs(packet -> time_end_from_S1 - packet -> time_start_in_S1);
		all_time_spent_in_S2 +=fabs(packet -> time_end_from_S2 - packet -> time_start_in_S2);
		if(packet -> time_end_from_S1 == 0){
			all_system_time += fabs(packet -> time_end_from_S2 - packet -> time_packet_arrival);
		}else{
			all_system_time += fabs(packet -> time_end_from_S1 - packet -> time_packet_arrival);
		}
		elem = My402ListNext(&Collect_List, elem);
		served_packet_num++;
	}

	if(served_packet_num == 0)
		served_packet_num++;
double average_time_in_system = all_system_time / served_packet_num / 1000;
while(!My402ListEmpty(&Collect_List))
	{
		elem = My402ListFirst(&Collect_List);
		packet = (My402Packet *)(elem -> obj);
		(void)My402ListUnlink(&Collect_List, elem);
		
		if(packet -> time_end_from_S1 == 0){
			 time_diff = fabs((packet -> time_end_from_S2 - packet -> time_packet_arrival)/1000);
			}else{
			 time_diff = fabs((packet -> time_end_from_S1 - packet -> time_packet_arrival)/1000);
			}
		double difference = time_diff - average_time_in_system;
		deviation = deviation + difference * difference;				
	}
	
	deviation /= served_packet_num;
    gettimeofday(&curr_time, 0);
    Final_Packet = timer(curr_time);
    fprintf(stdout, "%012.3fms: emulation ends\n", Final_Packet);		

		fprintf(stdout, "\nStatistics:\n");

	fprintf(stdout, "\n\taverage packet inter-arrival time = %.6gs\n", all_arrival_time / number_of_packets / 1000);
	fprintf(stdout, "\n\taverage packet service time = %.6gs\n", all_server_time / served_packet_num / 1000);
	fprintf(stdout, "\n\n\taverage number of packets in Q1 = %.6g s\n", all_time_spent_in_Q1/ (total_emulation_time));
	fprintf(stdout, "\n\taverage number of packets in Q2 = %.6g s\n", all_time_spent_in_Q2 / (total_emulation_time));
	fprintf(stdout, "\n\taverage number of packets in S1 = %.6g s\n", all_time_spent_in_S1 / (total_emulation_time));
	fprintf(stdout, "\n\taverage number of packets in S2 = %.6g s\n", all_time_spent_in_S2 / (total_emulation_time));
	fprintf(stdout, "\n\n\taverage time a packet spent in system = %.6gs\n", average_time_in_system);
	fprintf(stdout, "\n\tstandard deviation for time spent in system = %.6g\n", sqrt(deviation));
	fprintf(stdout, "\n\n\ttoken drop probability = %.6g\n", (double)Discard_Token / Token_ID);
	fprintf(stdout, "\n\tpacket drop probability = %.6g\n\n", (double)Discard_Packet / Packet_ID);
}
	pthread_exit(0);
}

void * packet_arrival(void * arg)
{

	int i;
	My402ListElem * elem;
	My402Packet * packet;
	struct timeval curr_time;
	double packet_present_stime, packet_previous_stime = 0;
	double sleep_time;
    sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset (&signal_mask, SIGUSR1);
	if(pthread_sigmask(SIG_UNBLOCK, &signal_mask, 0))
	printf("Packet arrival thread signal mask failed\n");
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = packet_handler;
	sigaction(SIGUSR1, &act, NULL);		
	if(emulation_mode != 't'){
        number_of_packets=n;
    }	
	
	for(i=0;i<number_of_packets;i++)
	{
		
		pthread_mutex_lock(&Mutex);
		if(emulation_mode=='t')
		{
			call();//to pass the element in file line by line
		}
		if(emulation_mode != 't'){
        prepare_dpacket();
    	}		
		if(My402ListEmpty(&Final_List)){//Number of elements in List is zero; unlock the mutex
			pthread_mutex_unlock(&Mutex);
		break;
		}
		elem = My402ListFirst(&Final_List);//This Returns a Pointer
		packet = (My402Packet *)(elem -> obj);//Dereference to get object
		Packet_ID=Packet_ID+1;//Increment the packet count
		if(Packet_ID>0){
			check_status=1;
		}
		pthread_mutex_unlock(&Mutex);


		gettimeofday(&curr_time, 0);//to obtain the time elapsed in the system since packet entered
		packet_present_stime = timer(curr_time);//returns the absolute time elapsed since start of function
		sleep_time = packet -> inter_arrival_time - (packet_present_stime - packet_previous_stime);//determine the sleep time of packet
		if(sleep_time > 0){
			sleep_time = sleep_time;
		}else{
			sleep_time = 1;//to handle if sleep - time becomes negative, sleep for 1mu sec.
		}
		usleep(sleep_time);


		pthread_mutex_lock(&Mutex);
		gettimeofday(&curr_time, 0);
		packet -> time_packet_arrival = timer(curr_time);//Packet wakes up after sleep this corresponds to the first field in file
		//printf("Tokens needed %d\n",packet -> tokens_need);
		if(B < packet -> tokens_need)//If the tokens needed exceeds the bucket depth drop the packet
		{
			(void)My402ListUnlink(&Final_List, elem);
			fprintf(stdout, "%012.3fms: p%ld arrives, needs %d tokens, dropped.\n", packet -> time_packet_arrival, Packet_ID, packet -> tokens_need);
			Discard_Packet=Discard_Packet+1;
			pthread_mutex_unlock(&Mutex);
			if(Packet_ID == number_of_packets && packet -> tokens_need > B){
				pthread_cond_broadcast(&Queue_Not_Empty);
				}			
			continue;
		}
		packet -> packet_num = Packet_ID;
		//printf("packet_num %d\n",packet -> packet_num);
		fprintf(stdout, "%012.3fms: p%ld arrives, needs %d tokens, inter-arrival time = %.3fms\n", packet -> time_packet_arrival, Packet_ID, packet -> tokens_need, packet -> time_packet_arrival - packet_previous_stime);
		packet_previous_stime = packet -> time_packet_arrival;			
		(void)My402ListAppend(&Q1, (void *)packet);//Append to the My402 list already created
		gettimeofday(&curr_time, 0);
		packet -> Start_Time_Q1 = timer(curr_time);//To determine the token_arrival_interval when the packet enters the queue Q1
		fprintf(stdout, "%012.3fms: p%ld enters Q1\n", packet -> Start_Time_Q1, packet -> packet_num);

		if(packet -> tokens_need <= temp_Bucket && (My402ListLength(&Q1) > 0)){//Tokens needed is less than bucket depth and Number of members in the queue greater than zero move packet

			My402ListElem * elem_temp = My402ListFirst(&Q1);
			(void)My402ListUnlink(&Q1, elem_temp);
			gettimeofday(&curr_time, 0);
			packet -> time_end_from_Q1 = timer(curr_time);//Timeout from Q2
			fprintf(stdout, "%012.3fms: p%ld leaves Q1, time in Q1 = %gms, token bucket now has %ld token\n", packet -> time_end_from_Q1, packet -> packet_num, packet -> time_end_from_Q1 - packet -> Start_Time_Q1, temp_Bucket);
			int broadcast_cond = My402ListEmpty(&Q2);
			//printf("broadcast_cond %d\n",broadcast_cond);
			temp_Bucket -= packet -> tokens_need;
			(void)My402ListAppend(&Q2, (void *)packet);
			Server_Guard = Server_Guard + 1;//Populated to evaluate when the server is woken up
			gettimeofday(&curr_time, 0);
			packet -> Start_Time_Q2 = timer(curr_time);
			fprintf(stdout, "%012.3fms: p%ld enters Q2\n", packet -> Start_Time_Q2, packet -> packet_num);
			if(broadcast_cond == 1)//To bradcast when there is incoming packet to the server
			{
				pthread_cond_broadcast(&Queue_Not_Empty);

			}
		}
		(void)My402ListUnlink(&Final_List, elem);
		pthread_mutex_unlock(&Mutex);		
	}
	return(0);
}

void * token_arrival(void * arg)
{
	double token_arrival_interval = ((1 /r) * 1000000);
	if(token_arrival_interval <= 0 || token_arrival_interval >= (10 * 1000000)){
		token_arrival_interval = 10 * 1000000;
	}else{
		token_arrival_interval=token_arrival_interval;
	}
	struct timeval curr_time;
	double token_arrival_time, token_present_stime, token_previous_stime = 0;
	double sleep_time;
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset (&signal_mask, SIGUSR2);
	if(pthread_sigmask(SIG_UNBLOCK, &signal_mask, 0))
		printf("Token arrival thread signal mask failed\n");

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = token_handler;
	sigaction(SIGUSR2, &act, NULL);	
	for(;;)
	{
		gettimeofday(&curr_time, 0);
		token_present_stime = timer(curr_time);//Token start time computed here.
		sleep_time = token_arrival_interval - (token_present_stime - token_previous_stime);
		//printf("token_arrival_interval %lf\n", token_arrival_interval);
		if(sleep_time > 0){
			sleep_time=sleep_time;
		}else{
			sleep_time = 1;
		}

		usleep(sleep_time);//Sleep according to the token arrival interval rate.
		//Increment token Count.


		pthread_mutex_lock(&Mutex);
		Token_ID=Token_ID+1;
		if((My402ListEmpty(&Final_List) && My402ListEmpty(&Q1))>0)//No packets in either Final_List and Q1
		{
			if(My402ListEmpty(&Q2) <= 0){//If there are any packets waiting in Q2 to be serviced broadcast the server
				pthread_cond_broadcast(&Queue_Not_Empty);
			}
			pthread_mutex_unlock(&Mutex);
			break;
		}
		gettimeofday(&curr_time, 0);
		token_arrival_time = timer(curr_time);
		if(B <= temp_Bucket)
		{
			Discard_Token=Discard_Token+1;
			
			fprintf(stdout, "%012.3fms: token t%ld arrives, dropped\n", token_arrival_time, Token_ID);
		}
		else
		{
			temp_Bucket=temp_Bucket+1;
			fprintf(stdout, "%012.3fms: token t%ld arrives, token bucket now has %ld token\n", token_arrival_time, Token_ID, temp_Bucket);
		}
		token_previous_stime = token_arrival_time;

		if(My402ListEmpty(&Q1)<1)//If the List created is not empty proceed else do Mutex Unlock
		{
			My402ListElem * elem = My402ListFirst(&Q1);
			My402Packet * packet = (My402Packet *)(elem -> obj);
			if(packet -> tokens_need <= temp_Bucket)
			{
				temp_Bucket = temp_Bucket - packet -> tokens_need;
				(void)My402ListUnlink(&Q1, elem);
				gettimeofday(&curr_time, 0);
				packet -> time_end_from_Q1 = timer(curr_time);
				fprintf(stdout, "%012.3fms: p%ld leaves Q1, time in Q1 = %gms, token bucket now has %ld token\n", packet -> time_end_from_Q1, packet -> packet_num, packet -> time_end_from_Q1 - packet -> Start_Time_Q1, temp_Bucket);
				int broadcast_cond = My402ListEmpty(&Q2);
				(void)My402ListAppend(&Q2, (void *)packet);
				gettimeofday(&curr_time, 0);
				packet -> Start_Time_Q2 = timer(curr_time);
				fprintf(stdout, "%012.3fms: p%ld enters Q2\n", packet -> Start_Time_Q2, packet -> packet_num);
				Server_Guard=Server_Guard+1;
				if(broadcast_cond == 1){
					pthread_cond_broadcast(&Queue_Not_Empty);
				}
			}
		}
		pthread_mutex_unlock(&Mutex);
	}
	return(0);
}



void * server1(void * arg)
{
	double temp = 0;
	struct timeval curr_time;
	while(1)
	{
		pthread_mutex_lock(&Mutex);
			while(My402ListEmpty(&Q2) == 1){
				if((Server_Interupt) || (My402ListEmpty(&Final_List) && My402ListEmpty(&Q1) && My402ListEmpty(&Q2) &&  (check_status == 1)) == 1){						packet_statistics();
				}
				pthread_cond_wait(&Queue_Not_Empty, &Mutex);		
		}
		if(Server_Interupt){
			packet_statistics();
		}		
		Server_Guard=Server_Guard-1;
		My402ListElem * elem = My402ListFirst(&Q2);
		My402Packet * packet = (My402Packet *)(elem -> obj);
		(void)My402ListUnlink(&Q2, elem);
		pthread_mutex_unlock(&Mutex);		
		gettimeofday(&curr_time, 0);
		packet -> time_start_in_S1 = timer(curr_time);
		packet -> time_end_from_Q2_S1 = packet -> time_start_in_S1;
		fprintf(stdout, "%012.3fms: p%ld leaves Q2, time in Q2 = %.3fms\n",
		packet -> time_start_in_S1, packet -> packet_num, packet -> time_end_from_Q2_S1 - packet -> Start_Time_Q2);
		
		gettimeofday(&curr_time, 0);
		temp = timer(curr_time);
		fprintf(stdout, "%012.3fms: p%ld begin service at S1, requesting %d ms of service\n",
		temp, packet -> packet_num,packet->service_time/1000);
		usleep(packet -> service_time);
		gettimeofday(&curr_time, 0);
		packet -> time_end_from_S1 = timer(curr_time);
		fprintf(stdout, "%012.3fms: p%ld departs from S1, service time = %.3fms, time in system = %.3fms\n",
			packet -> time_end_from_S1, packet -> packet_num, packet -> time_end_from_S1 - packet -> time_start_in_S1,
			packet -> time_end_from_S1 - packet -> time_packet_arrival);
		pthread_mutex_lock(&Mutex);
		(void)My402ListAppend(&Collect_List, (void *)packet);
		pthread_mutex_unlock(&Mutex);
			}
		
	return(0);
}

void * server2(void * arg)
{
    double temp = 0;
	struct timeval curr_time;
	while(1)  
	{
		pthread_mutex_lock(&Mutex);
			while(My402ListEmpty(&Q2) == 1){
				if((Server_Interupt) || (My402ListEmpty(&Final_List) && My402ListEmpty(&Q1) && My402ListEmpty(&Q2) &&  (check_status == 1)) == 1){

						packet_statistics();
										
				}
				pthread_cond_wait(&Queue_Not_Empty, &Mutex);		
		}
		if(Server_Interupt){
			packet_statistics();
		}		
		Server_Guard=Server_Guard-1;
		My402ListElem * elem = My402ListFirst(&Q2);
		My402Packet * packet = (My402Packet *)(elem -> obj);
		(void)My402ListUnlink(&Q2, elem);		
		pthread_mutex_unlock(&Mutex);
		gettimeofday(&curr_time, 0);
		packet -> time_start_in_S2 = timer(curr_time);
		packet -> time_end_from_Q2_S2 = packet -> time_start_in_S2;
		fprintf(stdout, "%012.3fms: p%ld leaves Q2, time in Q2 = %.3fms\n",
		packet -> time_start_in_S2, packet -> packet_num, packet -> time_end_from_Q2_S2 - packet -> Start_Time_Q2);

		gettimeofday(&curr_time, 0);
		temp = timer(curr_time);
		fprintf(stdout, "%012.3fms: p%ld begin service at S2, requesting %d ms of service\n",
		temp, packet -> packet_num,packet->service_time/1000);
		usleep(packet -> service_time);
		gettimeofday(&curr_time, 0);
		packet -> time_end_from_S2 = timer(curr_time);
		fprintf(stdout, "%012.3fms: p%ld departs from S2, service time = %.3fms, time in system = %.3fms\n",
			packet -> time_end_from_S2, packet -> packet_num, packet -> time_end_from_S2 - packet -> time_start_in_S2,
			packet -> time_end_from_S2 - packet -> time_packet_arrival);
		pthread_mutex_lock(&Mutex);
		(void)My402ListAppend(&Collect_List, (void *)packet);
		pthread_mutex_unlock(&Mutex);		
		}
	
	return(0);
}

void * Interupt_Handler(void * arg)
{
	sigwait(&Signal_Mask);
	pthread_mutex_lock(&Mutex);
	fprintf(stdout, "<Control + C Detected>\n");
	pthread_kill(packet_id, SIGUSR1);
	pthread_kill(token_id, SIGUSR2);
	Server_Interupt = 1;
	if(My402ListEmpty(&Q2))
		pthread_cond_broadcast(&Queue_Not_Empty);
	pthread_mutex_unlock(&Mutex);

	return(0);
}

int verify_data(char *temporary)
{
	int z;
	for (z=0; z<strlen(temporary) ; z++) {
		if(!isdigit(temporary[z])) {
			if(temporary[z] != '.')
				return 1;
		}
	}
	return 0;
}


int main(int argc, char * argv[]){

int j=1;
int l=0;
int k;
int m;
char *abuf;
double temp=0;
struct stat dir_check;
sigemptyset(&Signal_Mask);
sigaddset (&Signal_Mask, SIGINT);
sigaddset (&Signal_Mask, SIGUSR1);
sigaddset (&Signal_Mask, SIGUSR2);
if(pthread_sigmask(SIG_BLOCK, &Signal_Mask, 0))
	printf("Signal mask failed in main thread\n");

for(m=1;m<argc;m=m+2){
	
		l=l+2;
        abuf=argv[l];
        
        if(abuf == NULL){
        	printf("Malformed Command\n");
			printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");        	
        	exit(0);
        }
        
        if(strncmp(argv[m],"-t",strlen("-t"))!= 0)
        {
        for (k=0;k<strlen(abuf);k++) 
        {
            if(!isdigit((int)(abuf[k]))) 
            {
                if(abuf[k]!= '.')
                {
					printf("Malformed Command\n");
					printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");                    
                    exit(1);
                }
            }
        }
        }

}
for(j = 1; j < argc; j++){
	if(strcmp(argv[j], "-lambda")==0){
	j=j+1;
	if(j==argc){
		fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
		printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
		exit(1);
		}else{
			temp=verify_data(argv[j]);
			if(temp == 0){
			lambda=atof(argv[j]);
			if(lambda<0){
				printf("Lambda must be positive number. Please check the input\n");
				exit(0);
			}
		if((1/lambda)>10)
           	lambda=0.1;				
			}
			}
			}

		if(lambda == 0){
			lambda = 1;
			}

		if(strcmp(argv[j], "-mu")==0){
		j=j+1;
		if(j==argc) {
			fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
			printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
			exit(1);
		}else{
			temp=verify_data(argv[j]);	
			if(temp == 0){		
			mu=atof(argv[j]);
			if(mu<0){
				printf("Mu must be positive number. Please check the input\n");
				exit(0);
			}
		if((1/mu)>10)
           	mu=0.1;				
			}							
		}
		}

		if(mu == 0){
		mu = 0.35;
		}

		if(strcmp(argv[j], "-r")==0){
		j=j+1;
		if(j==argc) {
		fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
		printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
		exit(1);
		}else{
			temp=verify_data(argv[j]);
			if(temp == 0){
			r = strtod(argv[j],NULL);
			if(r<0){
				printf("r must be positive number. Please check the input\n");
				exit(0);
			}			
			if((double)(1/r) > 10)
				r=0.1;
				}
		}
		}
		if(r == 0){
			r = 1.5;
			}

		if(strcmp(argv[j], "-B")==0){
			j=j+1;
			if(j==argc) {
				fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
				printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}else{
				temp=verify_data(argv[j]);
				if(temp == 0){
				B = atof(argv[j]);
				if(B<0){
				printf("B must be positive number. Please check the input\n");
				exit(0);
				}	
				if(B>2147483647){
					printf("Value of B exceed the maximum specified range. Please check the input\n");
					exit(0);
				}
				}
			}
			}
			if(B == 0){
				B = 10;
				}

		if(strcmp(argv[j], "-P")==0){
			j=j+1;
			if(j==argc) {
				fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
				printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}else{
				temp=verify_data(argv[j]);
				if(temp == 0){
				P = atoi(argv[j]);
				if(P<0){
					printf("P must be positive number. Please check the input\n");
					exit(0);
				}	
				if(P>2147483647){
				printf("Value of P exceed the maximum specified range. Please check the input\n");
				exit(0);
				}
				}
			}
			}	
			if(P == 0){
				P = 3;
				}		

		if(strcmp(argv[j], "-n")==0){
			j=j+1;
			if(j==argc) {
				fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
				printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}else{
				temp=verify_data(argv[j]);
				if(temp == 0){
					n = atoi(argv[j]);
				if(n<0){
					printf("n must be positive number. Please check the input\n");
					exit(0);
				}	
				if(n>2147483647){
				printf("Value of n exceed the maximum specified range. Please check the input\n");
				exit(0);
				}			
				}
			}	
			}
			if(n == 0){
				n = 20;
				}			

		if(strcmp(argv[j], "-t")==0){
			j=j+1;
			if(j==argc) {
				fprintf(stderr, "\nNumber of Arguments are invalid. Please check the syntax of input.\n");
				printf("Syntax Format: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}else{
			fp = fopen(argv[j], "r");
			emulation_mode ='t';
			if(strcmp(argv[j],"/etc")==0){
				printf("malformed input - line 1 is not just a number\n");
				exit(0);
			}
	
			if((fp) == NULL){
			fprintf(stderr, "For the given path:%s\n",strerror(errno));
			exit(1);
			}			
				if(emulation_mode=='t'){
			call();
			}
			stat(argv[2], &dir_check);
			if(S_ISDIR(dir_check.st_mode)){
           	fprintf(stderr, "\"%s\" is a directory\n",argv[j]);
            exit(1);
        }							
			}			
		}	
	}
	fprintf(stdout, "\nEmulation Parameters:");
	fprintf(stdout, "\n\tnum = %ld\n\tlambda = %0.6g\n\tmu = %0.6g",n,lambda,mu);
    fprintf(stdout,"\n\tr = %0.6g\n\tB = %0.6g\n\tP = %0.6g\n\t\n",r,B,P);		
	memset(&Q1, 0, sizeof(My402List));
	(void)My402ListInit(&Q1);
	memset(&Q2, 0, sizeof(My402List));
	(void)My402ListInit(&Q2);
	memset(&Final_List, 0, sizeof(My402List));
	(void)My402ListInit(&Final_List);
	memset(&Collect_List, 0, sizeof(My402List));
	(void)My402ListInit(&Collect_List);
	gettimeofday(&Start_Time, 0);
	fprintf(stdout, "%012.3fms: emulation begins\n", timer(Start_Time));
	pthread_create(&packet_id, NULL, packet_arrival, 0);
	pthread_create(&token_id, NULL, token_arrival, 0);
	pthread_create(&server_id_1, 0, server1, 0);
	pthread_create(&server_id_2, 0, server2, 0);
	pthread_create(&interupt_id, 0, Interupt_Handler, 0);
	pthread_join(token_id, 0);
	pthread_join(packet_id, 0);		
	pthread_join(server_id_1, 0);	
	pthread_join(server_id_2, 0);
	if(emulation_mode=='t'){
			fclose(fp);
	}
   return(0);
}
