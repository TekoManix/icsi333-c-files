#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1000

int main(int argc, char *argv[]) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char *word;
    char filename[256];
    
    // Get filename from user if not provided as command line argument
    if (argc != 2) {
        printf("Enter filename: ");
        if (scanf("%255s", filename) != 1) {
            fprintf(stderr, "Error reading filename\n");
            return 1;
        }
    } else {
        strcpy(filename, argv[1]);
    }
    
    // Step 2: Open the file using fopen() function
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return 1;
    }
    
    printf("Reading file '%s' word by word:\n", filename);
    printf("================================\n");
    
    // Step 3: Process the file using suitable functions
    // Read the file line by line using fgets()
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Use strtok to split line into words (tokens separated by whitespace)
        word = strtok(line, " \t\n\r\f\v");
        
        // Print each word on a new line
        while (word != NULL) {
            printf("%s\n", word);
            word = strtok(NULL, " \t\n\r\f\v");
        }
    }
    
    // Step 4: Close the file using fclose() function
    fclose(fp);
    
    return 0;
}