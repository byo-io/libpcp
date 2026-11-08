// Compiled as C11 (see CMakeLists.txt), so this also checks that the public
// headers work there
#include "layout.h"

#define LAYOUT_VALUE(value) value,

size_t const layout_c11[] = {PCP_LAYOUT(LAYOUT_VALUE)};
