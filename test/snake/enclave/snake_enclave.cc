#include <openenclave/enclave.h>
#include "sgx_t.h"

int printf(char* ) { 
   int return_val;
oe_result_t oe_result = printf(&return_val, );
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall printf");
}
return return_val;
}

int printf(char* __format) { 
   int return_val;
oe_result_t oe_result = printf(&return_val, __format);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall printf");
}
return return_val;
}

int puts(char* __s) { 
   int return_val;
oe_result_t oe_result = puts(&return_val, __s);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall puts");
}
return return_val;
}

int rand() { 
   int return_val;
oe_result_t oe_result = rand(&return_val, );
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall rand");
}
return return_val;
}

void srand(unsigned int __seed) { 
   oe_result_t oe_result = srand(__seed);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall srand");
}
;
}

long time(long* __timer) { 
   long return_val;
oe_result_t oe_result = time(&return_val, __timer);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall time");
}
return return_val;
}

void show_score(screen_t* screen) { 
   oe_result_t oe_result = show_score(screen);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall show_score");
}
;
}

void draw_line(int col, int row) { 
   oe_result_t oe_result = draw_line(col, row);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ocall draw_line");
}
;
}

static oe_enclave_t* enclave = NULL;
