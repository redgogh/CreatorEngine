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

/* Create by Red Gogh on 2025/4/22 */

#pragma once

#include <unordered_map>
#include <algorithm>
#include <String.h>

template<typename K, typename V>
class HashMap : public std::unordered_map<K, V> {
public:
    using std::unordered_map<K, V>::unordered_map;

    void contains(const K& elem)
      {
        return std::find(this->begin(), this->end(), elem) != this->end();
      }

    void remove(const K& elem)
      {
        this->erase(std::remove(this->begin(), this->end(), elem), this->end());
      }

    V* find_ptr(const K& elem)
      {
        auto it = std::find(this->begin(), this->end(), elem);
        return it != this->end() ? &(*it) : nullptr;
      }

};