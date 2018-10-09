#include "spdlog/spdlog.h"

int main()
{
    auto console = spdlog::stdout_logger_mt("test");
    console->info("Info message");
    return 0;
}

