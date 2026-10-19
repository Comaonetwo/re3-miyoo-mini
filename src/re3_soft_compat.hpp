#pragma once
#ifndef RW_NULL
#error The re3 software compatibility header requires RW_NULL
#endif
namespace rw {namespace cpu {struct Im2DVertex;struct Im3DVertex;}}
#define RWDEVICE cpu
#include <rwcore.h>
#include "rw_soft_vertices.hpp"
