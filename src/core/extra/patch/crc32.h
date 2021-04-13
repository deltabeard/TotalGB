#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>


uint32_t GB_crc32(const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif
