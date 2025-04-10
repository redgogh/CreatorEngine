/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 RedGogh All rights reserved.                          *|
|*                                                                                  *|
|*    Licensed under the Apache License, Version 2.0 (the "License");               *|
|*    you may not use this file except in compliance with the License.              *|
|*    You may obtain a copy of the License at                                       *|
|*                                                                                  *|
|*        http://www.apache.org/licenses/LICENSE-2.0                                *|
|*                                                                                  *|
|*    Unless required by applicable law or agreed to in writing, software           *|
|*    distributed under the License is distributed on an "AS IS" BASIS,             *|
|*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.      *|
|*    See the License for the specific language governing permissions and           *|
|*    limitations under the License.                                                *|
|*                                                                                  *|
\* -------------------------------------------------------------------------------- */

/* Create by Red Gogh on 2025/4/7 */

#ifndef VERONICA_TYPE_DEF_H_
#define VERONICA_TYPE_DEF_H_

#include <memory>
#include <assert.h>
#include <stddef.h>

#if defined(__MINGW32__)
#  define U_MAYBE_UNUSED __attribute__((unused))
#else
#  define U_MAYBE_UNUSED
#endif

#define U_ASSERT_ONLY U_MAYBE_UNUSED

template<typename T, typename... Args>
inline static T* memnew(Args&& ... args)
{
        return new T(std::forward<Args>(args) ...);
}

inline static void memdel(void *ptr)
{
        delete ptr;
}

#endif /* VERONICA_TYPE_DEF_H_ */