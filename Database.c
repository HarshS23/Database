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

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND

}MetaCommandResult;

typedef enum {PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATMENT}PrepareResult; 

typedef enum {STATEMENT_INSERT, STATEMENT_SELECT}StatementType;

typedef struct{
    StatementType type;
}Statement;



// defining the input buffer 
InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer)); 
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0; 
    input_buffer->input_length = 0; 

    return input_buffer;
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer){
    if (strcmp(input_buffer->buffer, ".exit") == 0){
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
    if (strncmp(input_buffer->buffer, "insert", 6) == 0){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if(strcmp(input_buffer->buffer, "select") == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATMENT;

}

void execute_statement(Statement* statement){
    switch(statement->type){
        case (STATEMENT_INSERT):
            printf("This is where we do an Insert \n");
            break;
        case (STATEMENT_SELECT):
            printf("This is where we do an select \n");
            break;
    }
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


// Main function 
int main(int argc, char* argv[]){

    InputBuffer* input_buffer = new_input_buffer();

    while(true){
        print_prompt();
        read_input(input_buffer);

        if(strcmp(input_buffer->buffer, ".exit") == 0){
            close_input_buffer(input_buffer);
        }
        

        // checks the first char 
        // if it starts with a . then we know it is a command
        // otherwise user inputed a wrong command 
        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer)){
                case (META_COMMAND_SUCCESS):
                    continue;
                case (META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command!: %s\n", input_buffer->buffer); 
                    continue;
            }
        }
        //

        Statement statement;
        switch(prepare_statement(input_buffer, &statement)){
            case (PREPARE_SUCCESS):
                break; 
            case (PREPARE_UNRECOGNIZED_STATMENT):
                printf("Unrecognized Keyword at the start of '%s' . \n",input_buffer->buffer); 
                continue;
        }

        execute_statement(&statement); 
        printf("The statment has been exectued\n");

    }
}