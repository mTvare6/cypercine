#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>
#include <float.h>
#include <signal.h>
#include "argv.h"

typedef struct timespec tspec;

#define End          "\033[0;0m"
#define Bold         "\033[1m"
#define Dull         "\033[2m"
#define Italic       "\033[3m"
#define Underline    "\033[4m"
#define Inverted     "\033[7m"

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

typedef struct {
  char *execstr;
  size_t limit;
  double *range;
  double min, max, mode, total;
} command_t;

static const char *FILLEDBLOCK = "\xE2\x96\x88"; // U+2588 FULL BLOCK "█"
static const char *EMPTYBLOCK  = "\xE2\x96\x91"; // U+2591 LIGHT SHADE "░"

const bool nostdout=true;
static struct winsize w;
static long times=0, iterations=1;

void progress_bar(size_t i);
void new_cmd(command_t *cmd, char *execstr);
void handle_winch(int _){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
}

int main (int argc, char **argv){
  if(argc<2){
    puts("Usage:\n \tcypercine 100 'ls -lAth'\n\tcypercine 'exa' 'lsd' 'ls'\n\tcypercine 100000 'ls' 'lsd' 'exa'\n");
    exit(0);
  }

  setlocale(LC_ALL, "");

  if(setpgid(0,0)) {
    perror("failed to make proc group");
  }

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  signal(SIGWINCH, handle_winch);

  char *endptr = NULL;
  times = strtol(argv[1], &endptr, 10);

  bool setnumber=true;
  if (argv[1] == endptr) setnumber=false;

  times=(times==0)?1:times;

  size_t ncom=argc-1;
  if(setnumber)
    ncom--;

  command_t commands[ncom];

  for(size_t i=0;i<ncom;i++){
    new_cmd(&commands[i], argv[setnumber+i+1]);
  }

  if(ncom>1){
    printf("\n%sSummary%s(%smean%s)\n  ", Bold, End, LightBlue, End);
    for(size_t i=0;i<ncom;i++){
      printf("%s%s%s%s: \t%s%.2f%s\n  ", Bold, DarkCyan, commands[i].execstr, End, LightBlue, (commands[i].total/times)/1000, End);
    }
    puts("");
  }

  for(size_t i=0;i<ncom;i++)
    free(commands[i].range);

  return 0;
}

void progress_bar(size_t i){
  if (!isatty(STDOUT_FILENO)) return;
  fputs("\r", stdout);
  i += 1;
  long filled = (long)(((double)i / (double)times) * (double)w.ws_col);
  if (filled < 0) filled = 0;
  if (filled > (long)w.ws_col) filled = w.ws_col;

  for (long k = 0; k < filled; ++k) fputs(FILLEDBLOCK, stdout);
  for (long k = filled; k < (long)w.ws_col; ++k) fputs(EMPTYBLOCK, stdout);

  fflush(stdout);
}

void new_cmd(command_t *cmd, char *execstr){
  cmd->execstr = execstr;
  char **parsed_argv = argv_parse(cmd->execstr, &cmd->limit);
  char *null_argv[cmd->limit+1];
  for(int o=0;o<cmd->limit;o++){
    null_argv[o] = parsed_argv[o];
  }
  null_argv[cmd->limit] = NULL;
  tspec t1, t2;

  double timebuf;
  cmd->range = malloc(times * sizeof(*cmd->range));

  printf("%sBenchmark #%ld%s, %s\n", Bold, iterations, End, cmd->execstr);

  for(int i=0;i<times; i++){
    pid_t pid = fork();
    if(pid==-1) perror("fork()");
    else if(pid==0){
      if(nostdout){
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
      }
      execvp(parsed_argv[0], null_argv);
      perror("exec failed");
      exit(1);
    }
    else{
      int stat;
      clock_gettime(CLOCK_REALTIME, &t1);
      if(waitpid(pid, &stat, 0)<0){
        perror("Unable to wait for proc");
        exit(1);
      }
      if (WIFEXITED(stat)) {
        if(WEXITSTATUS(stat)!=0){
          printf("%s: exited with status %d", cmd->execstr, WEXITSTATUS(stat));
          exit(1);
        }
      } else {
        printf("command exited abnormally\n");
        exit(1);
      }
      if(nostdout)
        progress_bar(i);
      clock_gettime(CLOCK_REALTIME, &t2);
      timebuf = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_nsec - t1.tv_nsec) / 1e3;
      cmd->range[i]=timebuf;
      cmd->total+=timebuf;
    }
  }

  fputs("\033[2K\r", stdout);
  fflush(stdout);

  argv_free(parsed_argv);

  cmd->min=DBL_MAX;
  cmd->max=DBL_MIN;

  quicksort(cmd->range, 0, times-1);
  cmd->min=cmd->range[0];
  cmd->max=cmd->range[times-1];

  cmd->mode=cmd->range[0];
  double next;
  size_t curcount=1, nextcount=0;
  for(size_t i=0;i<times;i++){
    if(cmd->range[i]==cmd->mode) curcount++;
    else if(cmd->range[i]==next) nextcount++;
    else next=cmd->range[i];
    if(nextcount>curcount){
      cmd->mode=cmd->range[i];
      curcount=nextcount;
      nextcount=0;
    }
  }

  printf("Took %s%.2f%s ms\n  ", DarkCyan, (cmd->total/1000), End);
  printf("Time (%smean%s … %smode%s):\t%s%.2f%s … %s%.2f%s\n  ", LightGreen, End, DarkGreen, End, LightGreen,  (cmd->total/times)/1000, End, DarkGreen, cmd->mode/1000, End);
  printf("Range (%smin%s … %smax%s):\t%s%.2f%s … %s%.2f%s\n", LightMagenta, End, DarkMagenta, End, LightMagenta,  cmd->min/1000, End, DarkMagenta, cmd->max/1000, End);

  iterations++;
}
