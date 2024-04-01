#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
   
   const char* str[] = {"Hello", "Goodbye", "Morning"};
   size_t size = sizeof(str) / sizeof(str[0]);

   for (size_t i = 0; i < size; i++) {
     printf("%s\n", str[i]);
   }

   return EXIT_SUCCESS;
}
