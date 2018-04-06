#pragma once

#ifndef DCHECK
#define DCHECK(condition) assert(condition)
#endif

#ifndef CHECK
#define CHECK(condition) assert(condition)
#endif

#define CHECK_NOT_NULL(val) CHECK((val) != nullptr)
