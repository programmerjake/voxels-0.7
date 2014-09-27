#include "block/builtin/air.h"

const Blocks::builtin::Air *Blocks::builtin::Air::descriptor = nullptr;
initializer Blocks::builtin::Air::theInitializer(&Blocks::builtin::Air::init, &Blocks::builtin::Air::deinit);