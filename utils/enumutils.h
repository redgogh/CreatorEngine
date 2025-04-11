/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 Red Gogh All rights reserved.                          *|
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

/* Create by Red Gogh on 2025/4/8 */

#ifndef VRONK_ENUMUTILS_H_
#define VRONK_ENUMUTILS_H_

#include <magic_enum/magic_enum.hpp>

template<typename E>
inline static constexpr auto __magic_enum_name(const E& value)
{
        return magic_enum::enum_name(value);
}


#define MAGIC_ENUM_NAME(V) (std::string(__magic_enum_name(V)).c_str())

#endif /* VRONK_ENUMUTILS_H_ */
