#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
Right now we are making a simple REPL 
REPL = Read execute print loop 
this is for starting from the command line 

to do this we want our main function to have an infinite loop 
that prints the prompt 
*/

typedef struct{
    char* buffer;
    size_t buffer_length; // unsigned, represents size of object or # of bytes
    ssize_t input_length; // signed , represesnts size of objects or # of bytes

}InputBuffer;

// defining the input buffer 
InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer)); 
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0; 
    input_buffer->input_length = 0; 

    return input_buffer;
}

// defining the print prompt 
// prints a prompt to the user 
// DBC = DataBase Command 
void print_prompt() {
    printf("DBC > ");
}

void read_input(InputBuffer* input_buffer){
    ssize_t read_bytes = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin); 

    if (read_bytes <= 0){
        printf("FAILURE ERROR READING INPUT\n"); 
        exit(EXIT_FAILURE);
    }

    input_buffer->input_length = read_bytes - 1; 
    input_buffer->buffer[read_bytes - 1] = 0; 
}

void close_input_buffer(InputBuffer* input_buffer){
    free(input_buffer->buffer); 
    free(input_buffer); 
}

int main(int argc, char* argv[]){
    InputBuffer* input_buffer = new_input_buffer();

    while(true){
        print_prompt();
        read_input(input_buffer);

        if(strcmp(input_buffer->buffer, ".exit") == 0){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }else{
            printf("Command not Recognized: '%s'\n", input_buffer->buffer);
        }
    }
}