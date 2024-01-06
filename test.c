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

  printf("t2: %s\n", t2);
  assert(*t1 == 1);

  char* t3 = smalloc(4096);
  assert(t3 != NULL);
  strcpy(t3, "This is a perfectly normal sentence!");

  printf("%s\n", t3);

  printf("Freeing!\n");
  sfree(t1);
  sfree(t2);
  sfree(t3);

  //sfree((void *)0xDEADBEEF);

  /* Only test these if using test smalloc()! */
//  char *t3 = smalloc(1023 * sizeof(Header));
//  assert(t3 != NULL);
//  sfree(t3);
//
}
