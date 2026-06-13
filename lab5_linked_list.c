#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define the node structure for integers
struct intNode {
    int data;
    struct intNode *next;
};

// Function to create a new node
struct intNode* createNode(int value) {
    struct intNode *newNode = (struct intNode*)malloc(sizeof(struct intNode));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newNode->data = value;
    newNode->next = NULL;
    return newNode;
}

// Function to add a node to the end of the list
void addToList(struct intNode **head, int value) {
    struct intNode *newNode = createNode(value);
    
    // If list is empty, make new node the head
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    
    // Otherwise, traverse to the end and add the new node
    struct intNode *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = newNode;
}

// Function to print the linked list
void printList(struct intNode *head) {
    printf("Generated numbers in the list: ");
    struct intNode *current = head;
    
    if (current == NULL) {
        printf("(empty list)\n");
        return;
    }
    
    while (current != NULL) {
        printf("%d", current->data);
        if (current->next != NULL) {
            printf(" -> ");
        }
        current = current->next;
    }
    printf("\n");
}

// Function to free all nodes in the list
void freeList(struct intNode *head) {
    struct intNode *current = head;
    struct intNode *temp;
    
    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp);
    }
}

int main() {
    struct intNode *head = NULL;
    int randomNum;
    
    // Initialize random number generator with current time
    srand(time(0));
    
    printf("Generating random numbers between 0 and 50...\n");
    printf("Will stop when 49 is generated.\n\n");
    
    while (1) {
        // Generate random number between 0 and 50
        randomNum = rand() % 51;
        
        printf("Generated: %d\n", randomNum);
        
        // If we generate 49, print the list and exit
        if (randomNum == 49) {
            printf("\nGenerated 49! Printing the list and exiting...\n");
            printList(head);
            freeList(head);
            break;
        }
        
        // Otherwise, add the number to the list
        addToList(&head, randomNum);
    }
    
    return 0;
}