#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
 
// updated for Ubuntu added SDL/ dir 
#include <SDL/SDL.h>
 
// defaults
#define PROB_TREE 0.0009
#define PROB_FIRE 0.001
 
#define TIMERFREQ 10

#define SMOKEJUMPERS_K 100
 
#ifndef WIDTH
#  define WIDTH 250
#endif
#ifndef HEIGHT
#  define HEIGHT 250
#  define MAXSIZE WIDTH*HEIGHT
#endif
#ifndef BPP
#  define BPP 32
#endif
 
#define MAX_TIMESTEP 5000

#if BPP != 32
  #warning This program could not work with BPP different from 32
#endif
 
uint8_t *field[2], swapu;
double prob_f = PROB_FIRE, prob_tree = PROB_TREE; 
int smokejumpers_k = SMOKEJUMPERS_K;
int num_fires = 0;
int num_trees = 0;
unsigned int *fire_log;
bool gui_enabled = true;

enum cell_state { 
  VOID, TREE, BURNING
};

int step = 0, longevity = 0, biomass = 0;
// simplistic random func to give [0, 1)
double prand()
{
  return (double)rand() / (RAND_MAX + 1.0);
}
 
// initialize the field
void init_field(void)
{
  int i, j;
  swapu = 0;

  for(i = 0; i < WIDTH; i++)
  {
    for(j = 0; j < HEIGHT; j++)
    {     
      // CS523 proj gl called for Initial state of the model is for all cells be empty
      field[0][j*WIDTH + i] = VOID;
      fire_log[j*WIDTH + i] = 0;
    }
  }
}


int avg(int num1, int num2){
    return (num1 + num2)/2;
}


void update_biomass(uint8_t *field){
    int i, j, tree_counter = 0;
    for(i = 0; i < WIDTH; i++)
        for(j = 0; j < HEIGHT; j++)
           if(field[j*WIDTH + i] == TREE)
               tree_counter++;
    biomass = avg(biomass, tree_counter);
}


bool is_field_empty(uint8_t *field){
  int i,j;
  for(i = 0; i < WIDTH; i++)
      for(j = 0; j < HEIGHT; j++)
          if(field[j*WIDTH + i] != VOID)
              return false;
  return true;
}
 
// the "core" of the task: the "forest-fire CA"
bool burning_neighbor(int, int);
pthread_mutex_t synclock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t simulate(uint32_t iv, void *p)
{
  int i, j, k;
  int temp, loc;
 
  /*
    Since this is called by SDL, "likely"(*) in a separated
    thread, we try to avoid corrupted updating of the display
    (done by the show() func): show needs the "right" swapu
    i.e. the right complete field. (*) what if it is not so?
    The following is an attempt to avoid unpleasant updates.
   */
  pthread_mutex_lock(&synclock);

  num_trees = 0;
  k = num_fires;
  for (i = 0; i < smokejumpers_k && k >= 0; i++) {
     temp = k > 0 ? rand()%k : 0;
     loc = *(fire_log + temp);
     *(field[swapu] + loc) = TREE;
     //printf("FF %i working on Fire %i at %i made TREE\n",i,temp,loc);
     *(fire_log + temp) = *(fire_log + k);
     *(fire_log + k) = 0;
     k--;
  }
  for ( ; k >= 0; k-- )  {
     *(fire_log + k) = 0;
  }

  step++;
  longevity++;
  if(step > 1 && is_field_empty(field[swapu])){
      printf("\n forest is empty: %d", step);fflush(stdout);
      step = MAX_TIMESTEP; // in order to exit the main loop
  }
  update_biomass(field[swapu]);

  //printf("longevity:%d,biomass:%d\n", longevity, biomass);

  k = 0; // fire counter

  for(i = 0; i < WIDTH; i++) {
    for(j = 0; j < HEIGHT; j++) {
      enum cell_state s = *(field[swapu] + j*WIDTH + i);
      switch(s)
      {
      case BURNING:
	*(field[swapu^1] + j*WIDTH + i) = VOID;
	break;
      case VOID:
	*(field[swapu^1] + j*WIDTH + i) = prand() > prob_tree ? VOID : TREE;
        // should we add if not first tree, prand for 2nd tree?
	break;
      case TREE:
        num_trees++;
	if (burning_neighbor(i, j)) {
	  *(field[swapu^1] + j*WIDTH + i) = BURNING;
          *(fire_log + k) = j*WIDTH + i;
            //printf("new fires no. %i at %i\n", k, j*WIDTH+i);
            k++;
          }
          else {
	    //*(field[swapu^1] + j*WIDTH + i) = prand() > prob_f ? TREE : BURNING;
            if ( prand() < prob_f ) {
	      *(field[swapu^1] + j*WIDTH + i) = BURNING;
              /* *(fire_log + k) = j*WIDTH + i;
              printf("new fires no. %i at %i\n", k, j*WIDTH+i);
              k++;*/
            } else *(field[swapu^1] + j*WIDTH + i) = TREE;
        }
	break;
      default:
	fprintf(stderr, "corrupted field\n");
	break;
      }
    }
  }
  num_fires = k - 1;
  //printf("Number of trees %i, and number of fires %i\n", num_trees, num_fires + 1);

  swapu ^= 1;
  pthread_mutex_unlock(&synclock);
  return iv;
}
 

// the field is a "part" of an infinite "void" region
#define NB(I,J) (((I)<WIDTH)&&((I)>=0)&&((J)<HEIGHT)&&((J)>=0) \
		 ? (*(field[swapu] + (J)*WIDTH + (I)) == BURNING) : false)

// look at neighbor formula, does it match this?
bool burning_neighbor(int i, int j)
{
  return NB(i-1,j-1) || NB(i-1, j) || NB(i-1, j+1) ||
    NB(i, j-1) || NB(i, j+1) ||
    NB(i+1, j-1) || NB(i+1, j) || NB(i+1, j+1);
}
 
 
// "map" the field into gfx mem
// burning trees are red
// trees are green
// "voids" are black;
void show(SDL_Surface *s)
{
  int i, j;
  uint8_t *pixels = (uint8_t *)s->pixels;
  uint32_t color;
  SDL_PixelFormat *f = s->format;
 
  pthread_mutex_lock(&synclock);
  for(i = 0; i < WIDTH; i++) {
    for(j = 0; j < HEIGHT; j++) {
      switch(*(field[swapu] + j*WIDTH + i)) {
      case VOID:
	color = SDL_MapRGBA(f, 0,0,0,255);
	break;
      case TREE:
	color = SDL_MapRGBA(f, 0,255,0,255);
	break;
      case BURNING:
	color = SDL_MapRGBA(f, 255,0,0,255);
	break;
      }
      *(uint32_t*)(pixels + j*s->pitch + i*(BPP>>3)) = color;
    }
  }
  pthread_mutex_unlock(&synclock);
}
 
int main(int argc, char **argv)
{
  SDL_Surface *scr = NULL;
  SDL_Event event[1];
  bool quit = false, running = false;
  SDL_TimerID tid;
 
  // add variability to the simulation
  srand(time(NULL));
 
  // we can change prob_tree, but prob_f is fixed
  if (argc > 1 ) {
     prob_tree = atof(argv[1]);
     if (argc > 2 ) {
       smokejumpers_k = atoi(argv[2]);
     } else smokejumpers_k = SMOKEJUMPERS_K;
     if (argc > 3 ) {
       if(atoi(argv[3]) == 0)
           gui_enabled = false;
     } else gui_enabled = true;
  } else prob_tree = PROB_TREE;
 
  //printf("prob_f %lf\nprob_tree %lf\nratio %lf\n\n", 
	// prob_f, prob_tree, prob_tree/prob_f);
 
  if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0 ) return EXIT_FAILURE;
  atexit(SDL_Quit);
 
  field[0] = malloc(WIDTH*HEIGHT);
  if (field[0] == NULL) exit(EXIT_FAILURE);
  field[1] = malloc(WIDTH*HEIGHT);
  if (field[1] == NULL) { free(field[0]); exit(EXIT_FAILURE); }
 
  fire_log = malloc(WIDTH*HEIGHT*sizeof(unsigned int));
  if (fire_log == NULL) exit(EXIT_FAILURE);
  
  if (gui_enabled){
      scr = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, SDL_HWSURFACE|SDL_DOUBLEBUF);
      if (scr == NULL) {
        fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
        free(field[0]); free(field[1]);
        exit(EXIT_FAILURE);
      }
  }
  init_field();

  tid = SDL_AddTimer(TIMERFREQ, simulate, NULL); // suppose success
  running = true;
 
  event->type = SDL_VIDEOEXPOSE;
  SDL_PushEvent(event);
 
  while((SDL_WaitEvent(event) && !quit) && step < MAX_TIMESTEP)
  {
    switch(event->type)
    {
    case SDL_VIDEOEXPOSE:
      if (gui_enabled){
          while(SDL_LockSurface(scr) != 0) SDL_Delay(1);
          show(scr);
          SDL_UnlockSurface(scr);
          SDL_Flip(scr);
      }
      event->type = SDL_VIDEOEXPOSE;
      SDL_PushEvent(event);
      break;
    case SDL_KEYDOWN:
      switch(event->key.keysym.sym)
      {
      case SDLK_q:
	quit = true;
	break;
      case SDLK_p:
	if (running)
	{
	  running = false;
	  pthread_mutex_lock(&synclock);
	  SDL_RemoveTimer(tid); // ignore failure...
	  pthread_mutex_unlock(&synclock);
	} else {
	  running = true;
	  tid = SDL_AddTimer(TIMERFREQ, simulate, NULL);
	  // suppose success...
	}
	break;
      }
    case SDL_QUIT:
      quit = true;
      break;
    }
  }
  printf("\nlongevity:%d,biomass:%d\n", longevity, biomass);
  if (running) {
    pthread_mutex_lock(&synclock);
    SDL_RemoveTimer(tid);
    pthread_mutex_unlock(&synclock);
  }
  free(field[0]); free(field[1]);
  exit(EXIT_SUCCESS);
}
