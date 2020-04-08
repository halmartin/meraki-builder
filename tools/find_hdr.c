#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

// Adapted from: https://stackoverflow.com/a/21191228
size_t le_find(FILE *fi, uint8_t *shdr, size_t size, bool footer){
    size_t i, index = 0;
    int ch;
    long int shdr_pos = -1;
    // while we aren't at the end of the file
    while(EOF!=(ch=fgetc(fi))){
	// compare the first character
        if(ch == shdr[index]){
            // print the matched byte in hex
	    //printf("byte: %hhX\n", ch);
	    //printf("pos: %ld\n", ftell(fi));
            // we matched, so increment index
            if(++index == size){
                if (footer) {
                    // return the file position at the end of the footer
                    shdr_pos = ftell(fi);
                } else {
                    // returnt the file position at the start of the header
                    shdr_pos = ftell(fi) - index;
		}
            }
        } else {
	    // reset the index
            index = 0;
        }
    }
    return shdr_pos;
}

// Adapted from https://stackoverflow.com/a/24479532
int main( int argc, char *argv[] )  {
   FILE *file;
   bool gzip = false;
   bool xz = false;
   bool footer = false;
   int opt;
   while ((opt = getopt(argc, argv, "gxf")) != -1) {
       switch(opt) {
           case 'g': gzip = true; break;
           case 'x': xz = true; break;
           case 'f': footer = true; break;
           default:
               fprintf(stderr, "Usage: %s [-gxf] [file...]\n", argv[0]);
               exit(EXIT_FAILURE);
       }
   }
   if (optind < argc) { 
       file = fopen(argv[optind],"rb");
       size_t le_pos;
       if (gzip) {
	   // gzip has no footer, so the option is a NOP for gzip
           uint8_t shdr[7] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00};
           le_pos = le_find(file, shdr, sizeof(shdr), 0);
       } else if (xz) {
           if (footer) {
	       // from 2.1.1.1 in https://tukaani.org/xz/xz-file-format.txt
               // I would match on {0x00, 0x01, 0x59, 0x5a} but this fails for some reason :(
               uint8_t shdr[3] = {0x01, 0x59, 0x5a};
               le_pos = le_find(file, shdr, sizeof(shdr), footer);
           } else {
               // from 2.1.1.1 in https://tukaani.org/xz/xz-file-format.txt
               uint8_t shdr[6] = {0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00};
               le_pos = le_find(file, shdr, sizeof(shdr), footer);
	   }
       } else {
           fprintf(stderr, "Usage: %s [-gxf] [file...]\n", argv[0]);
           exit(EXIT_FAILURE);
       }

       printf("%ld\n", le_pos);
       return 0;
   }
   fprintf(stderr, "Usage: %s [-gx] [file...]\n", argv[0]);
   return 1;
}
