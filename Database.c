#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

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

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATMENT,
    PREPARE_SYNTAX_ERROR
}PrepareResult; 


typedef enum {STATEMENT_INSERT, STATEMENT_SELECT}StatementType;

typedef enum {EXECUTE_SUCCESS , EXECUTE_TABLE_FULL}ExecuteResult; 

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
}Row;


typedef struct{
    StatementType type;
    Row row_to_insert;
}Statement;

/*

plan  
    - store each block of memeory called pages 
    - each page stores as many rows as it can fit 
    - Rows are searlized into compact representation of each page 
    - pages are only allocated as needed
    - Keep a fixed size array of pointers to pages

*/
 

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row,id); 
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email); 
const uint32_t ID_OFFSET = 0; 
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE; 
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE; 

// table structure that points to pages of rows and keeps track of how many rows there are 
// we make the page 4 kilobytes and for one page in our database means its one page in our operating systems 
// This also means that when the operating system moves the pages in and out of memeory it moves whole units 
// rather than in bits this ensures 
#define TABLE_MAX_PAGES 100
const uint32_t PAGE_SIZE = 4096; 
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE; 
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct{
    uint32_t num_rows; 
    Pager* pager;
}Table;


typedef struct{
    int file_discriptor;
    uint32_t file_length; 
    void* pages[TABLE_MAX_PAGES];
}Pager;


void* get_page(Pager* pager, uint32_t page_num){
    if (page_num > TABLE_MAX_PAGES){
        printf("Unable to fetch page, page out of index (%d > %d)\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE); 
    }

    if (pager->pages[page_num] == NULL){ 
        // cache miss. allocate from memory and load from file 
        void* page = malloc(PAGE_SIZE); 
        uint32_t num_pages = pager->file_length / PAGE_SIZE; 

        // we might have a partial page at the end of the file 
        if (pager->file_length % PAGE_SIZE){
            num_pages += 1; 
        } 

        if (page_num <= num_pages){
            lseek(pager->file_discriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_discriptor, page, PAGE_SIZE);
            if (bytes_read == -1){
                printf("Error reading file %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = page;

    }

    return pager->pages[page_num]; 

}









void print_row(Row* row){
    printf("(%d %s %s)\n", row->id, row->username, row->email);
}

void close_input_buffer(InputBuffer* input_buffer){
    free(input_buffer->buffer); 
    free(input_buffer); 
}

void free_table(Table* table){
    for(int i = 0; table->pages[i]; i++){
        free(table->pages[i]);
    }
    free(table); 
}



void serialize_row(Row* source, void* destination){
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE); 
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);

}

void deserialize_row(void *source, Row* destination){
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE); 
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE); 
}





// row slots: reading and writing into memory for a particular row 
void* row_slot(Table* table, uint32_t row_num){
    uint32_t page_num = row_num / ROWS_PER_PAGE; 

    void* page = get_page(table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset; 
}


// defining the input buffer 
InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer)); 
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0; 
    input_buffer->input_length = 0; 

    return input_buffer;
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table){
    if (strcmp(input_buffer->buffer, ".exit") == 0){
        close_input_buffer(input_buffer);
        free_table(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// replaceing scanf with strtok (string token) because it breaks up strings into small parts called 
// tokens. if we use scanf and move on we will cause a buffer overflow due to the the buffer size of scanf
// if the string is larger than the buffer than it will cause buffer overflow, strtok will break up a string into smaller 
// tokens and store in the buffer. 

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement){

    statement->type = STATEMENT_INSERT; 

    char* keyword = strtok(input_buffer->buffer , " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " "); 
    char* email = strtok(NULL, " "); 

    if (id_string == NULL || username == NULL || email == NULL){
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string); // atoi() converts a string into an interger 
    if (id < 0){
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE){
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE){
        return PREPARE_STRING_TOO_LONG; 
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username); 
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS; 

}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){

    if (strncmp(input_buffer->buffer, "insert", 6) == 0){
        return prepare_insert(input_buffer, statement);
    }

    if(strcmp(input_buffer->buffer, "select") == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATMENT;

}

ExecuteResult execute_insert(Statement* statement, Table* table){
    if (table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert); 

    serialize_row(row_to_insert, row_slot(table, table->num_rows)); 
    table->num_rows += 1; 

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row; 

    for(uint32_t i = 0; i < table->num_rows; i++){
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }

    return EXECUTE_SUCCESS; 
}

ExecuteResult execute_statement(Statement* statement, Table* table){
    switch(statement->type){
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table); 
    }
}

// lastly we need to initlize the table and free the memory 


Pager* pager_open(const char* filename){ 
    int fd = open(filename, O_RDWR // read and write 
                          | O_CREAT, // Create file if i doenst exit
                           S_IWUSR |  // user write premission 
                           S_IRUSR      // user read premission
                         );


    if (fd == -1){ 
        printf("Unable to open file\n"); 
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END); 

    Pager* pager = malloc(sizeof(Pager));

    pager->file_discriptor = fd; 
    pager->file_length = file_length; 

    for(int i = 0; i < TABLE_MAX_PAGES; i++){
        pager->pages[i] = NULL;
    }

    return pager; 
}



Table* db_open(const char* filename){
    Pager* pager = pager_open(filename); 
    uint32_t num_rows = pager->file_length / ROW_SIZE;

    Table* table = malloc(sizeof(Table)); 
    table->pager = pager; 
    table->num_rows = num_rows; 
    return table; 
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


// Main function 
int main(int argc, char* argv[]){

    Table* table = new_table();

    InputBuffer* input_buffer = new_input_buffer();

    while(true){
        print_prompt();
        read_input(input_buffer);

        if(strcmp(input_buffer->buffer, ".exit") == 0){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }

        // checks the first char 
        // if it starts with a . then we know it is a command
        // otherwise user inputed a wrong command 
        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer,table)){
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
            case (PREPARE_NEGATIVE_ID):
                printf("ID must be postive\n");
                continue;
            case (PREPARE_STRING_TOO_LONG):
                printf("This string is too long\n"); 
                continue;
            case (PREPARE_SYNTAX_ERROR):
                continue;
                printf("Syntax Error, could not parse command\n");
            case (PREPARE_UNRECOGNIZED_STATMENT):
                printf("Unrecognized Keyword at the start of '%s' . \n",input_buffer->buffer); 
                continue;
        }

        // execute_statement(&statement);
        switch (execute_statement(&statement, table)){
            case(EXECUTE_SUCCESS):
                printf("Executed\n");
                break; 
            case(EXECUTE_TABLE_FULL):
                printf("Error: Table Full\n");
                break;
        }

    }
}