#pragma once

#if defined RP_DEBUG && RP_DEBUG >= 1
	#define dbg_verify() verify()
#else
	#define dbg_verify()
#endif

#if defined RP_ENV_PER
#include "env-per.h"
#elif defined RP_ENV_BAS
#include "env-bas.h"
#else
#include "env-iti.h"
#endif
