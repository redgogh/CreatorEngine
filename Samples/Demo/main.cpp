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

#include <Engine/Engine.h>

// std
#include <stdio.h>
#include <stdlib.h>

int main()
{
    system("chcp 65001 >nul");

    /*
     * close stdout and stderr write to buf, let direct
     * output.
     */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    Gogh_Engine_Init(800, 600, "Game");
    
    while (!Gogh_Engine_IsShouldClose()) {
        Gogh_Engine_PollEvents();
        Gogh_Engine_BeginNewFrame();
        Gogh_Engine_EndNewFrame();
    }
 
    Gogh_Engine_Terminate();
    
}