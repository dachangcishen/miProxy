#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

//find the start of the first occurrence of the substring 'needle' in the string 'haystack'
//to find "\r\n\r\n", needle = "\r\n\r\n", needlelen = 4
//return a pointer of the first occurence
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen){

    char *hpointer = haystack;
    
    while (haystacklen >= needlelen ){
        if (!memcmp(hpointer, needle, needlelen)){
            return (void *)hpointer;
        }
        haystacklen--;
        hpointer++;
    }

    return NULL;
}

// get the data of key in the header lines, store the result into val
// to get the content length, key = "Content-Length", keylen = 14
// if successfully get: return 1, otherwise: return 0
int get_header_val(char *head, size_t headlen, char *key, size_t keylen, char *val){
    char *string;

    string = memmem(head, headlen, key, keylen);
    if (string == NULL) return 0;
    
    size_t remaining_length = headlen - (string - head);
    string = memmem(string, remaining_length, ":", 1);
    if (string == NULL) return 0;

    string += 2;

    remaining_length = remaining_length - (string - head);
    if (memmem(string, remaining_length, "\n", 1) == NULL) return 0;
    long int t = 0;
    for (t = 0; string[t] != '\n'; t++){
        val[t] = string[t];
    }
    val[t - 1] = '\0'; 

    return 1;
}

