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


// very epic spacing thanks to vim easy align
#define End          "\033[0;0m"
#define Bold         "\033[1m"
#define Dull         "\033[2m"
#define Italic       "\033[3m"
#define Underline    "\033[4m"
#define Inverted     "\033[7m" // or reverse according to wikipedia

#define DarkBlack    "\033[30m"
#define DarkRed      "\033[31m"
#define DarkGreen    "\033[32m"
#define DarkYellow   "\033[33m"
#define DarkBlue     "\033[34m"
#define DarkMagenta  "\033[35m"
#define DarkCyan     "\033[36m"
#define DarkWhite    "\033[37m"
#define LightBlack   "\033[90m"
#define LightRed     "\033[91m"
#define LightGreen   "\033[92m"
#define LightYellow  "\033[93m"
#define LightBlue    "\033[94m"
#define LightMagenta "\033[95m"
#define LightCyan    "\033[96m"
#define LightWhite   "\033[97m"


static int argv_set(char *args, char **argv);
static char **argv_parse(char *args, size_t *argc);
static void argv_free(char **argv);




void handle_winch(int sig);

wchar_t FILLEDBLOCK=L'█';
wchar_t EMPTYBLOCK=L'░';
struct winsize w;
long times=0;

typedef struct {
  char **argv;
  tspec start, end;
  size_t lim;
  double timebuf, total;
  double min, max;
  double range[];
} command_t;

void progress_bar(size_t i);
float compare (const void * a, const void * b) {
  return ( *(float*)a - *(float*)b );
}

void quicksort(double *x,double first,double last)
{   int pivot, j, i;
  double temp;

  if(first<last){
    pivot=first;
    i=first;
    j=last;


    while(i<j){
      while(x[i]<=x[pivot]&&i<last)
        i++;
      while(x[j]>x[pivot])
        j--;
      if(i<j){
        temp=x[i];
        x[i]=x[j];
        x[j]=temp;
      }
    }

    temp=x[pivot];
    x[pivot]=x[j];
    x[j]=temp;
    quicksort(x,first,j-1);
    quicksort(x,j+1,last);
  }
}

int main (int argc, char **argv){

  if(argc<2){ printf("Usage: cyperfine 100 'ls -lAth'"); exit(0); }

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

  double freq[times];
  double ct1, total;
  pid_t pid;
  double c1range[times]; // for mean and mode
  printf("%sBenchmark #%d%s, %s\n", Bold, 1, End, argv[argc-1]);

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

  printf("\r%*s\r", w.ws_col, "");
  argv_free(c1);

  double min=sizeof(double),max=0;

  /* quicksort(c1range, 0, times-1); */
  /* min=c1range[0]; */
  /* max=c1range[times-1]; */

  double mode=c1range[0], next;
  size_t curcount=1, nextcount;
  for(int i=0;i<times;i++){
    if(c1range[i]<min) min=c1range[i];
    if(c1range[i]>max) max=c1range[i];
    if(c1range[i]==mode) curcount++;
    else if(c1range[i]==next) nextcount++;
    else next=c1range[i];
    if(nextcount>curcount){
      mode=c1range[i];
      curcount=nextcount;
      nextcount=0;
    }
  }

  printf("Took %s%.2f%s ms\n", DarkCyan, (total/1000), End);
  printf("%*s", 2, "");
  printf("Time (%smean%s … %smode%s):\t%s%.2f%s … %s%.2f%s\n", LightGreen, End, DarkGreen, End, LightGreen,  (total/times)/1000, End, DarkGreen, mode/1000, End);
  printf("%*s", 2, "");
  printf("Range (%smin%s … %smax%s):\t%s%.2f%s … %s%.2f%s\n", LightMagenta, End, DarkMagenta, End, LightMagenta,  min/1000, End, DarkMagenta, max/1000, End);

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


