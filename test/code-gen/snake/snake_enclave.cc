#include "interface_selectors.h"
#include "asylo/platform/primitives/extent.h"
#include "asylo/platform/primitives/primitive_status.h"
#include "asylo/platform/primitives/trusted_primitives.h"
#include "asylo/util/status_macro.h"

extern void show_score(screen_t* screen);

extern void setup_level(screen_t* screen, snake_t* snake, int level);

extern void move(snake_t* snake, char * keys, char key);

extern int collide_object(snake_t* snake, screen_t* screen, char object);

extern int collision(snake_t* snake, screen_t* screen);

extern int eat_gold(snake_t* snake, screen_t* screen);

namespace asylo {

namespace primitives {

extern "C" PrimitiveStatus asylo_enclave_init() { 
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(keat_goldEnclaveSelector, EntryHandler{secure_eat_gold}));
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(kcollisionEnclaveSelector, EntryHandler{secure_collision}));
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(kcollide_objectEnclaveSelector, EntryHandler{secure_collide_object}));
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(kshow_scoreEnclaveSelector, EntryHandler{secure_show_score}));
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(kmoveEnclaveSelector, EntryHandler{secure_move}));
    ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler(ksetup_levelEnclaveSelector, EntryHandler{secure_setup_level}));
   return PrimitiveStatus::OkStatus();
}

extern "C" PrimitiveStatus asylo_enclave_fini() { 
   return PrimitiveStatus::OkStatus();
}

namespace  {

PrimitiveStatus Abort(void* context, TrustedParameterStack* params) { 
   TrustedPrimitives::BestEffortAbort("Aborting enclave");
   return PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_show_score(void* context, TrustedParameterStack* params) { 
   screen screen_param = params->Pop<screen_t>();
   show_score(&screen_param);;
   *params->PushAlloc<screen_t>() = screen_param;
   PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_setup_level(void* context, TrustedParameterStack* params) { 
   level level_param = params->Pop<int>();
   snake snake_param = params->Pop<snake_t>();
   screen screen_param = params->Pop<screen_t>();
   setup_level(&screen_param, &snake_param, level_param);;
   *params->PushAlloc<screen_t>() = screen_param;
   *params->PushAlloc<snake_t>() = snake_param;
   *params->PushAlloc<int>() = level_param;
   PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_move(void* context, TrustedParameterStack* params) { 
   key key_param = params->Pop<char>();
   keys keys_param = params->Pop<char *>();
   snake snake_param = params->Pop<snake_t>();
   move(&snake_param, keys_param, key_param);;
   *params->PushAlloc<snake_t>() = snake_param;
   *params->PushAlloc<char *>() = keys_param;
   *params->PushAlloc<char>() = key_param;
   PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_collide_object(void* context, TrustedParameterStack* params) { 
   object object_param = params->Pop<char>();
   screen screen_param = params->Pop<screen_t>();
   snake snake_param = params->Pop<snake_t>();
   int returnVal = collide_object(&snake_param, &screen_param, object_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<snake_t>() = snake_param;
   *params->PushAlloc<screen_t>() = screen_param;
   *params->PushAlloc<char>() = object_param;
   PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_collision(void* context, TrustedParameterStack* params) { 
   screen screen_param = params->Pop<screen_t>();
   snake snake_param = params->Pop<snake_t>();
   int returnVal = collision(&snake_param, &screen_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<snake_t>() = snake_param;
   *params->PushAlloc<screen_t>() = screen_param;
   PrimitiveStatus::OkStatus();
}

PrimitiveStatus secure_eat_gold(void* context, TrustedParameterStack* params) { 
   screen screen_param = params->Pop<screen_t>();
   snake snake_param = params->Pop<snake_t>();
   int returnVal = eat_gold(&snake_param, &screen_param);;
   *params->PushAlloc<int>() = returnVal;
   *params->PushAlloc<snake_t>() = snake_param;
   *params->PushAlloc<screen_t>() = screen_param;
   PrimitiveStatus::OkStatus();
}

} // namespace 
} // namespace primitives
} // namespace asylo
