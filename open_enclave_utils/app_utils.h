#include <openenclave/host.h>

oe_enclave_t* enclave = NULL;

bool check_simulate_opt(int* argc, const char* argv[])
{
    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "--simulate") == 0)
        {
            fprintf(stdout, "Running in simulation mode\n");
            memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
            (*argc)--;
            return true;
        }
    }
    return false;
}

void terminate_enclave()
{
    if (enclave)
        oe_terminate_enclave(enclave);
}

void create_enclave(int argc, const char* argv[])
{
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    if (check_simulate_opt(&argc, argv))
    {
        flags |= OE_ENCLAVE_FLAG_SIMULATE;
    }

    if (argc != 2)
    {
        fprintf(
            stderr, "Usage: %s enclave_image_path [ --simulate  ]\n", argv[0]);
	terminate_enclave();
	return;
    }

    oe_result_t result = oe_create_sgx_enclave(
        argv[1], OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "oe_create_sgx_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
	terminate_enclave();
	return;
    }
}

