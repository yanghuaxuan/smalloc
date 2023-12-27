#include "smalloc.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {

  int *t1 = smalloc(4);
  *t1 = 1;
  assert(*t1 == 1);

  char *t2 = smalloc(420);
  strcpy(t2, "Hello world");

  printf("%s", t2);

  sfree(t1);
  sfree(t2);

  /* Only test these if using test smalloc()! */
//  char *t3 = smalloc(1023 * sizeof(Header));
//  assert(t3 != NULL);
//  sfree(t3);
//
  char *t4 = smalloc(1023 * sizeof(Header));
  char *t5 = smalloc(1024 * sizeof(Header));
  sfree(t4);
  sfree(t5);
}
