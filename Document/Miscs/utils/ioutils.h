/* ======================================================================== */
/* ioutils.h                                                                */
/* ======================================================================== */
/*                        This file is part of:                             */
/*                            BRIGHT ENGINE                                 */
/* ======================================================================== */
/*                                                                          */
/* Copyright (C) 2022 Vcredent All rights reserved.                         */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License");          */
/* you may not use this file except in compliance with the License.         */
/*                                                                          */
/* You may obtain a copy of the License at                                  */
/*     http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied  */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/*                                                                          */
/* ======================================================================== */
#ifndef _VRONK_IOUTILS_H_
#define _VRONK_IOUTILS_H_

#include <stdio.h>
#include <fstream>

static char *io_bytebuf_read(const char *path, size_t *size)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("error open file failed!");

    *size = file.tellg();
    file.seekg(0);

    /* malloc buffer */
    char *buf = (char *) malloc(*size);
    file.read(buf, *size);
    file.close();

    return buf;
}

static void io_bytebuf_free(char *buf)
{
    free(buf);
}

#endif /* _VRONK_IOUTILS_H_ */
