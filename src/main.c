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

#define DarkBlack "\033[30m"
#define DarkRed "\033[31m"
#define DarkGreen "\033[32m"
#define DarkYellow "\033[33m"
#define DarkBlue "\033[34m"
#define DarkMagenta "\033[35m"
#define DarkCyan "\033[36m"
#define DarkWhite "\033[37m"
#define LightBlack "\033[90m"
#define LightRed "\033[91m"
#define LightGreen "\033[92m"
#define LightYellow "\033[93m"
#define LightBlue "\033[94m"
#define LightMagenta "\033[95m"
#define LightCyan "\033[96m"
#define LightWhite "\033[97m"
#define End "\033[0;0m"


static int argv_set(char *args, char **argv);
static char **argv_parse(char *args, size_t *argc);
static void argv_free(char **argv);



void handle_winch(int sig);

wchar_t FILLEDBLOCK=L'█';
wchar_t EMPTYBLOCK=L'░';
struct winsize w;
long times=0;

void progress_bar(size_t i);

int main (int argc, char **argv){

  if(setpgid(0,0)) {
    perror("failed to make proc group");
  }
  setlocale(LC_ALL, ""); // progress bar unicode 
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  /* signal(SIGWINCH, handle_winch); */

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

  printf("Took %s%.2f%s ms\n", DarkCyan, (total/1000), End);
  printf("Average %s%.2f%s ms\n", LightCyan, (total/times)/1000, End);
  printf("min: %s%.2f%s\n", LightMagenta, min/1000, End);
  printf("max: %s%.2f%s\n", DarkMagenta, max/1000, End);

  return 0;
}




void progress_bar(size_t i){

  putchar('\r');
  fflush(stdout);
  i+=1;
  long tsa=(((double)i/times))*w.ws_col;
  for(i=0;i<tsa;i++){
    putwchar(FILLEDBLOCK);
  }
  for(i=tsa;i<w.ws_col;i++){
    putwchar(EMPTYBLOCK);
  }
}

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

static char **argv_parse(char *args, size_t *argc)
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

static void argv_free(char **argv)
{
  if (argv) {
    free(argv[-1]);
    free(argv-1);
  } 
}

void handle_winch(int sig){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // term width height
}


