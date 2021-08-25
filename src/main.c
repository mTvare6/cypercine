#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

typedef struct timespec tspec;

typedef struct {
  tspec start;
  tspec end;
} elapsed_counter_t;

static int setargs(char *args, char **argv)
{
  int count = 0;

  while (isspace(*args)) ++args;
  while (*args) {
    if (argv) argv[count] = args;
    while (*args && !isspace(*args)) ++args;
    if (argv && *args) *args++ = '\0';
    while (isspace(*args)) ++args;
    count++;
  }
  return count;
}

char **parsedargs(char *args, size_t *argc)
{
  char **argv = NULL;

  if (args && *args
      && (args = strdup(args))
      && (*argc = setargs(args,NULL))
      && (argv = malloc((*argc+1) * sizeof(char *)))) {
    *argv++ = args;
    *argc = setargs(args,argv);
  }

  if (args && !argv) free(args);
  return argv;
}

void freeparsedargs(char **argv)
{
  if (argv) {
    free(argv[-1]);
    free(argv-1);
  } 
}

int main (int argc, char **argv){

  if(setpgid(0,0)) {
    perror("failed to make proc group");
  }

  size_t lim_c1;
  char **c1 = parsedargs(argv[argc-1], &lim_c1);
  tspec t1, t2;
  

  long times=0;
  times = strtol(argv[1], NULL, 10);
  times=(times==0)?1:times;

  double ct1, total;
  pid_t pid;
  double c1range[times];

  for(int i=0;i<times; i++){
    pid = fork();
    if(pid==-1) perror("fork()");
    else if(pid==0){
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
      execvp(c1[0], c1);
    }
    else{
      int stat;
      clock_gettime(CLOCK_REALTIME, &t1);
      waitpid(pid, &stat, 0);
      clock_gettime(CLOCK_REALTIME, &t2);
      ct1 = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_nsec - t1.tv_nsec) / 1e3;
      c1range[i]=ct1;
      total+=ct1;
    }
  }
  if(pid==0) return 0;
  freeparsedargs(c1);
  printf("Took %.2f ms", (total/1000));
  putchar('\n');
  for(int i=0;i<times;i++){
    printf("%.2f ", (c1range[i])/1000);
  }
  putchar('\n');

  return 0;
}



