#pragma once

#include <cstdint>
#include "asylo/platform/primitives/primitives.h"

namespace asylo {

namespace primitives {

static constexpr uint64_t keat_goldEnclaveSelector = kSelectorUser + 6
static constexpr uint64_t kcollisionEnclaveSelector = kSelectorUser + 5
static constexpr uint64_t kcollide_objectEnclaveSelector = kSelectorUser + 4
static constexpr uint64_t kshow_scoreEnclaveSelector = kSelectorUser + 1
static constexpr uint64_t kmoveEnclaveSelector = kSelectorUser + 3
static constexpr uint64_t ksetup_levelEnclaveSelector = kSelectorUser + 2

static constexpr uint64_t kdraw_lineOcallHandler = kSelectorUser + 6
static constexpr uint64_t ktimeOcallHandler = kSelectorUser + 5
static constexpr uint64_t ksrandOcallHandler = kSelectorUser + 4
static constexpr uint64_t krandOcallHandler = kSelectorUser + 3
static constexpr uint64_t kprintfOcallHandler = kSelectorUser + 0
static constexpr uint64_t kputsOcallHandler = kSelectorUser + 2
} // namespace asylo
} // namespace primitives
