#include <ctype.h>
#include <stdlib.h>
#include <string.h>
size_t argv_set(char *args, char **argv)
{
  size_t count = 0;

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


void quicksort(double *x,double first,double last){
  int pivot, j, i;
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

