#include <stddef.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>

typedef struct timespec tspec;


wchar_t FILLEDBLOCK=L'█';
wchar_t EMPTYBLOCK=L'░';
typedef struct {
  tspec start;
  tspec end;
} elapsed_counter_t;

static int argv_set(char *args, char **argv)
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

char **argv_parse(char *args, size_t *argc)
{
  char **argv = NULL;

  if (args && *args
      && (args = strdup(args))
      && (*argc = argv_set(args,NULL))
      && (argv = malloc((*argc+1) * sizeof(char *)))) {
    *argv++ = args;
    *argc = argv_set(args,argv);
  }

  if (args && !argv) free(args);
  return argv;
}

void argv_free(char **argv)
{
  if (argv) {
    free(argv[-1]);
    free(argv-1);
  } 
}

struct winsize w;
long times=0;

void handle_winch(int sig){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // term width height
}

void progress_bar(size_t i);

int main (int argc, char **argv){

  if(setpgid(0,0)) {
    perror("failed to make proc group");
  }
  setlocale(LC_ALL, ""); // progress bar unicode 
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  signal(SIGWINCH, handle_winch);

  size_t lim_c1;
  char **c1 = argv_parse(argv[argc-1], &lim_c1);
  tspec t1, t2;



  times = strtol(argv[1], NULL, 10);
  times=(times==0)?1:times; // if ascii string or 0, set to 1

  double ct1, total;
  pid_t pid;
  double c1range[times]; // for mean and mode

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
      progress_bar(i);
      waitpid(pid, &stat, 0);
      clock_gettime(CLOCK_REALTIME, &t2);
      ct1 = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_nsec - t1.tv_nsec) / 1e3;
      c1range[i]=ct1;
      total+=ct1;
    }
  }
  if(pid==0) return 0;

  putchar('\n');
  argv_free(c1);

  double min=sizeof(double),max=0;
  for(int i=0;i<times;i++){
    if(c1range[i]<min) min=c1range[i];
    if(c1range[i]>max) max=c1range[i];
  }

  printf("Took %.2f ms\n", (total/1000));
  printf("Average %.2f ms\n", (total/times)/1000);
  printf("min:%.2f\nmax:%.2f\n", min/1000, max/1000);

  return 0;
}




void progress_bar(size_t i){

  putchar('\r');
  fflush(stdout);
  long tsa=((double)i/times)*w.ws_col;
  for(int i=0;i<=tsa+1;i++){
    putwchar(FILLEDBLOCK);
  }
  for(int i=tsa;i<w.ws_col-3;i++){
    putwchar(EMPTYBLOCK);
  }
}
