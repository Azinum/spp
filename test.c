// test.c

#include <stdlib.h>
#include <stdio.h>

int main(void) {
  $(int a) -> void {
    while (a--) {
      if (a == 5) {
        printf("OK\n");
      }
    }
  }(10);
  return 0;
}
