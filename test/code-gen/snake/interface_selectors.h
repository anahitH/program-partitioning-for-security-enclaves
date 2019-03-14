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
static constexpr uint64_t kAbortEnclaveSelector = kSelectorUser6
static constexpr uint64_t kdraw_lineOCallHandler = kSelectorUser + 6
static constexpr uint64_t ktimeOCallHandler = kSelectorUser + 5
static constexpr uint64_t ksrandOCallHandler = kSelectorUser + 4
static constexpr uint64_t krandOCallHandler = kSelectorUser + 3
static constexpr uint64_t kprintfOCallHandler = kSelectorUser + 0
static constexpr uint64_t kputsOCallHandler = kSelectorUser + 2
} // namespace primitives
} // namespace asylo
