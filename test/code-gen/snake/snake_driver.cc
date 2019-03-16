DEFINE_string(enclave_path, "", "Path to enclave binary");

#include "interface_selectors.h"
#include "gflags/gflags.h"
#include "asylo/platform/primitives/extent.h"
#include "asylo/platform/primitives/sim/untrusted_sim.h"
#include "asylo/platform/primitives/untrusted_primitives.h"
#include "asylo/platform/primitives/primitive_status.h"
#include "asylo/platform/primitives/util/dispatch_table.h"
#include "asylo/platform/primitives/trusted_primitives.h"
#include "asylo/util/status_macro.h"

extern int app_main();

extern int printf(char* );

extern int printf(char* __format);

extern int puts(char* __s);

extern int rand();

extern void srand(unsigned int __seed);

extern long time(long* __timer);

extern void draw_line(int col, int row);

int main(int argc, char* argv) { 
   google::ParseCommandLineFlags(&argc, &argv, true);
   asylo::primitives::registerOCalls();
   app_main();
   return 0;
}

void show_score(screen_t* screen) { 
   asylo::primitives::show_score(&screen);;
   return returnVal;
}

void setup_level(screen_t* screen, snake_t* snake, int level) { 
   asylo::primitives::setup_level(&screen, &snake, level);;
   return returnVal;
}

void move(snake_t* snake, char * keys, char key) { 
   asylo::primitives::move(&snake, keys, key);;
   return returnVal;
}

int collide_object(snake_t* snake, screen_t* screen, char object) { 
   int returnVal;
   asylo::primitives::collide_object(&snake, &screen, object, &returnVal);;
   return returnVal;
}

int collision(snake_t* snake, screen_t* screen) { 
   int returnVal;
   asylo::primitives::collision(&snake, &screen, &returnVal);;
   return returnVal;
}

int eat_gold(snake_t* snake, screen_t* screen) { 
   int returnVal;
   asylo::primitives::eat_gold(&snake, &screen, &returnVal);;
   return returnVal;
}

namespace asylo {

namespace primitives {

Status registerOCalls() { 
   ASYLO_ASSIGN_OR_RETURN(client, LoadEnclave<SimBackend>(FLAGS_enclave_path, absl::make_unique<DispatchTable>()));;
   Status status;
   status = client->exit_call_provider()->RegisterExitHandler(kprintfOCallHandler, ExitHandler{app_printf});
   status = client->exit_call_provider()->RegisterExitHandler(kprintfOCallHandler, ExitHandler{app_printf});
   status = client->exit_call_provider()->RegisterExitHandler(kputsOCallHandler, ExitHandler{app_puts});
   status = client->exit_call_provider()->RegisterExitHandler(krandOCallHandler, ExitHandler{app_rand});
   status = client->exit_call_provider()->RegisterExitHandler(ksrandOCallHandler, ExitHandler{app_srand});
   status = client->exit_call_provider()->RegisterExitHandler(ktimeOCallHandler, ExitHandler{app_time});
   status = client->exit_call_provider()->RegisterExitHandler(kdraw_lineOCallHandler, ExitHandler{app_draw_line});
}

Status app_printf(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
    _param = params->Pop<char>();
   int returnVal = printf(&_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<char>() = _param;
   PrimitiveStatus::OkStatus();
}

Status app_printf(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   __format __format_param = params->Pop<char>();
   int returnVal = printf(&__format_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<char>() = __format_param;
   PrimitiveStatus::OkStatus();
}

Status app_puts(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   __s __s_param = params->Pop<char>();
   int returnVal = puts(&__s_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<char>() = __s_param;
   PrimitiveStatus::OkStatus();
}

Status app_rand(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   int returnVal = rand();;
   *params->PushAlloc<int>() = returnVal;
   PrimitiveStatus::OkStatus();
}

Status app_srand(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   __seed __seed_param = params->Pop<unsigned int>();
   srand(__seed_param);;
   *params->PushAlloc<unsigned int>() = __seed_param;
   PrimitiveStatus::OkStatus();
}

Status app_time(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   __timer __timer_param = params->Pop<long>();
   long returnVal = time(&__timer_param);;
   *params->PushAlloc<long>() = returnVal;
   *params->PushAlloc<long>() = __timer_param;
   PrimitiveStatus::OkStatus();
}

Status app_draw_line(std::shared_ptr<EnclaveClient> client, void* context, UntrustedParameterStack* params) { 
   row row_param = params->Pop<int>();
   col col_param = params->Pop<int>();
   draw_line(col_param, row_param);;
   *params->PushAlloc<int>() = col_param;
   *params->PushAlloc<int>() = row_param;
   PrimitiveStatus::OkStatus();
}

Status show_score(screen_t* screen) { 
   UntrustedParameterStack params;
   *params.PushAlloc<screen_t>() = *screen;
   auto status = client->EnclaveCall(kshow_scoreEnclaveSelector, &params);
   *screen = params.Pop<screen_t>();
   return Status::OkStatus();
}

Status setup_level(screen_t* screen, snake_t* snake, int level) { 
   UntrustedParameterStack params;
   *params.PushAlloc<screen_t>() = *screen;
   *params.PushAlloc<snake_t>() = *snake;
   *params.PushAlloc<int>() = level;
   auto status = client->EnclaveCall(ksetup_levelEnclaveSelector, &params);
   *screen = params.Pop<screen_t>();
   *snake = params.Pop<snake_t>();
   level = params.Pop<int>();
   return Status::OkStatus();
}

Status move(snake_t* snake, char * keys, char key) { 
   UntrustedParameterStack params;
   *params.PushAlloc<snake_t>() = *snake;
   *params.PushAlloc<char *>() = keys;
   *params.PushAlloc<char>() = key;
   auto status = client->EnclaveCall(kmoveEnclaveSelector, &params);
   *snake = params.Pop<snake_t>();
   keys = params.Pop<char *>();
   key = params.Pop<char>();
   return Status::OkStatus();
}

Status collide_object(snake_t* snake, screen_t* screen, char object, int* returnVal) { 
   UntrustedParameterStack params;
   *params.PushAlloc<snake_t>() = *snake;
   *params.PushAlloc<screen_t>() = *screen;
   *params.PushAlloc<char>() = object;
   *params.PushAlloc<int>() = *returnVal;
   auto status = client->EnclaveCall(kcollide_objectEnclaveSelector, &params);
   *snake = params.Pop<snake_t>();
   *screen = params.Pop<screen_t>();
   object = params.Pop<char>();
   *returnVal = params.Pop<int>();
   return Status::OkStatus();
}

Status collision(snake_t* snake, screen_t* screen, int* returnVal) { 
   UntrustedParameterStack params;
   *params.PushAlloc<snake_t>() = *snake;
   *params.PushAlloc<screen_t>() = *screen;
   *params.PushAlloc<int>() = *returnVal;
   auto status = client->EnclaveCall(kcollisionEnclaveSelector, &params);
   *snake = params.Pop<snake_t>();
   *screen = params.Pop<screen_t>();
   *returnVal = params.Pop<int>();
   return Status::OkStatus();
}

Status eat_gold(snake_t* snake, screen_t* screen, int* returnVal) { 
   UntrustedParameterStack params;
   *params.PushAlloc<snake_t>() = *snake;
   *params.PushAlloc<screen_t>() = *screen;
   *params.PushAlloc<int>() = *returnVal;
   auto status = client->EnclaveCall(keat_goldEnclaveSelector, &params);
   *snake = params.Pop<snake_t>();
   *screen = params.Pop<screen_t>();
   *returnVal = params.Pop<int>();
   return Status::OkStatus();
}

static std::shared_ptr<EnclaveClient> client
} // namespace primitives
} // namespace asylo
