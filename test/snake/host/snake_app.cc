#include "sgx_u.h"
#include "app_utils.h"

extern int app_main();

int main(int argc, const char*[] argv) { 
   create_enclave(argc, argv);
   app_main();
   return 0;
}

void setup_level(screen_t* screen, snake_t* snake, int level) { 
   oe_result_t oe_result = setup_level(enclave, screen, snake, level);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ecall setup_level");
terminate_enclave();
exit(1);
}
;
}

void move(snake_t* snake, char[] keys, char key) { 
   oe_result_t oe_result = move(enclave, snake, keys, key);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ecall move");
terminate_enclave();
exit(1);
}
;
}

int collide_object(snake_t* snake, screen_t* screen, char object) { 
   int return_val;
oe_result_t oe_result = collide_object(enclave, &return_val, snake, screen, object);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ecall collide_object");
terminate_enclave();
exit(1);
}
return return_val;
}

int collision(snake_t* snake, screen_t* screen) { 
   int return_val;
oe_result_t oe_result = collision(enclave, &return_val, snake, screen);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ecall collision");
terminate_enclave();
exit(1);
}
return return_val;
}

int eat_gold(snake_t* snake, screen_t* screen) { 
   int return_val;
oe_result_t oe_result = eat_gold(enclave, &return_val, snake, screen);
if (oe_result != OE_OK) {
fprintf(stderr, "Failed to invoke ecall eat_gold");
terminate_enclave();
exit(1);
}
return return_val;
}

