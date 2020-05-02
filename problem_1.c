#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>
#include<unistd.h>
#include<assert.h>
#include<limits.h>
#include<stdint.h>

pthread_cond_t* cats_cv;
pthread_cond_t* dogs_cv;
pthread_cond_t* birds_cv;

volatile int cats=0,dogs=0,birds=0, play_time= 1;

int num_cats = 0, num_dogs=0, num_birds=0;
/* Monitor that protects shared variable count*/
typedef struct animal_monitor{
	pthread_mutex_t* lock;
	int tid;
	int pet_type;
	int count;
}animal_monitor;

/* When cat is entering the playground, make sure no dog and birds are there, If they are there, then cat should wait. */
void cat_enter(animal_monitor* thread){
        pthread_mutex_lock(thread->lock);
        
	while(dogs != 0 || birds != 0){
		pthread_cond_wait(cats_cv, thread->lock);
	}
	/* Waiting is over, cat can now enter */
        cats++;
        thread->count++;
        pthread_mutex_unlock(thread->lock);
}

void cat_exit(animal_monitor* thread){
	pthread_mutex_lock(thread->lock);
	/* A cat exited. Reduce its count */
	if(cats>=1){
		cats--;
	}
	/* If there are no cats, birds and dogs can enter the playground. So signal them. */
	if(cats == 0){
		pthread_cond_broadcast(birds_cv);
	        pthread_cond_broadcast(dogs_cv);
	}
	pthread_mutex_unlock(thread->lock);
}

/* When a dog is entering the playground, make sure there are no cats in the playground, If they are there, then the dog should wait. */
void dog_enter(animal_monitor* thread){
	pthread_mutex_lock(thread->lock);
	/* Wait if there are cats in the playground */
	while(cats != 0){
		pthread_cond_wait(dogs_cv, thread->lock);
	}
	/* Now a dog can play. */
	dogs++;
	thread->count++;
	pthread_mutex_unlock(thread->lock);
}

void dog_exit(animal_monitor* thread){
	pthread_mutex_lock(thread->lock);
	/* A dog just exited. Reduce its count */
	if(dogs >= 1){
		dogs--;
	}
	/* Notify waiting cats that there are no dogs. */
	if(dogs == 0){
		pthread_cond_broadcast(cats_cv);
	}
	pthread_mutex_unlock(thread->lock);	
}
/* Make sure there are no cats when a bird is entering */
void bird_enter(animal_monitor* thread){
	pthread_mutex_lock(thread->lock);
	/*printf("Birds entered!\n");*/
	/* Wait if there are any cats */
	while(cats != 0){
		pthread_cond_wait(birds_cv, thread->lock);
	}
	/* A bird can now play */
	birds++;
	thread->count++;
	pthread_mutex_unlock(thread->lock);
}

void bird_exit(animal_monitor* thread){
	pthread_mutex_lock(thread->lock);
	/* A bird just exited. Decrement it's count */
	if(birds >= 1){
		birds--;
	}
	/* Notify waiting cats when there are no birds */
	if(birds == 0){
		pthread_cond_broadcast(cats_cv);
	}
	pthread_mutex_unlock(thread->lock);	
}

void play(void) {
	int i = 0;
	for(i=0; i<10; i++) {
    		assert(cats >= 0 && cats <= num_cats);
    		assert(dogs >= 0 && dogs <= num_dogs);
    		assert(birds >= 0 && birds <= num_birds);
    		assert(cats == 0 || dogs == 0);
    		assert(cats == 0 || birds == 0);
   	}
}
/* cats, dogs and birds have pet_type  2,1,0 respectively. Call their respective enter(), exit functions based on pet type.*/
void* thread_start_routine(void* arg){
	animal_monitor* animal = ((animal_monitor*)arg);
	if(animal->pet_type == 2){
		while(play_time == 1){
			cat_enter(animal);
			play();
			cat_exit(animal);
		}
	}
	
	if(animal->pet_type == 1){
		while(play_time == 1){
			dog_enter(animal);
			play();
			dog_exit(animal);
		}
	}
	
	if(animal->pet_type == 0){
		while(play_time == 1){
			/*printf("Birds playing\n");*/
			bird_enter(animal);
			play();
			bird_exit(animal);
		}
	}
	return NULL;
}


int main(int argc, char* argv[])
{
	if(argc != 4){
		fprintf(stderr, "Error, Please provide correct number of arguements \n");
		return 1;
	}
	num_cats = atoi(argv[1]);
	num_dogs = atoi(argv[2]);
	num_birds = atoi(argv[3]);

	/*Check if their numbers are correct*/
	if(num_birds < 0 || num_birds > 99 || num_cats < 0 || num_cats > 99 || num_dogs < 0 || num_dogs > 99)
	{
		fprintf(stderr, "Please enter the input numbers in the range 0 - 99\n");
		return 0;
	}

	int total = num_birds + num_cats + num_dogs;

	pthread_t total_threads[total];
	animal_monitor animal_thread[total];
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cv_cats;
  	cats_cv = &cv_cats;
	
	pthread_cond_t cv_dogs;
	dogs_cv = &cv_dogs;

	pthread_cond_t cv_birds;
	birds_cv = &cv_birds;
	
	pthread_cond_init(cats_cv, NULL);
	pthread_cond_init(dogs_cv, NULL);
	pthread_cond_init(birds_cv, NULL);
	
	int i = 0;
	for(i = 0; i < total; i++){
		animal_thread[i].tid = i;
		animal_thread[i].lock = &lock;
		if(i < num_cats) animal_thread[i].pet_type = 2;
		else if(i >= num_cats && i < (total - num_birds)) animal_thread[i].pet_type = 1;
		else animal_thread[i].pet_type = 0;
		animal_thread[i].count = 0;
		int thread_creation_fail = pthread_create(&total_threads[i], NULL, thread_start_routine, &animal_thread[i]);
		if(thread_creation_fail){
			printf("Error, Sorry, Threads could not be creted\n");
			return -1;
		}
	}
	sleep(10);
	play_time = 0;
	for(i = 0; i < total; i++)
	{
		pthread_join(total_threads[i], NULL);
	}
		
	int cats_play = 0, dogs_play = 0, birds_play = 0;	
	
	for(i = 0; i < total; i++){
		if(i < num_cats) cats_play += animal_thread[i].count;
		else if(i >= num_cats && i < (total - num_birds)) dogs_play += animal_thread[i].count;
		else birds_play += animal_thread[i].count;
	}
	printf("cat play = %i, dog play = %i, bird play = %i\n", cats_play, dogs_play, birds_play);
	return 1; 
}
