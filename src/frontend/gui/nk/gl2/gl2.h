#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../interface.h"


struct NkInterface* nk_interface_gl2_init(
	const struct NkInterfaceInitConfig* config
);

#ifdef __cplusplus
}
#endif
