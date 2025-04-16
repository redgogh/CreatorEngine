@REM  ======================================================================== ::
@REM                                                                           ::
@REM  Copyright (C) 2022 Vcredent All rights reserved.                         ::
@REM                                                                           ::
@REM  Licensed under the Apache License, Version 2.0 (the "License");          ::
@REM  you may not use this file except in compliance with the License.         ::
@REM                                                                           ::
@REM  You may obtain a copy of the License at                                  ::
@REM      http://www.apache.org/licenses/LICENSE-2.0                           ::
@REM                                                                           ::
@REM  Unless required by applicable law or agreed to in writing, software      ::
@REM  distributed under the License is distributed on an "AS IS" BASIS,        ::
@REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, e1ither express or implied ::
@REM  See the License for the specific language governing permissions and      ::
@REM  limitations under the License.                                           ::
@REM                                                                           ::
@REM  ======================================================================== ::
@echo off

set root=../

glslc -fshader-stage=vert %root%/shaders/glsl/simple_vertex.glsl -o %root%/shaders/spir-v/simple_vertex.spv
glslc -fshader-stage=frag %root%/shaders/glsl/simple_fragment.glsl -o %root%/shaders/spir-v/simple_fragment.spv

robocopy %root%/shaders/spir-v %root%/cmake-build-debug/shaders/spir-v /e /copyall /r:0 /w:0 >nul