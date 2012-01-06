/*
 * libmm-session
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungbae Shin <seungbae.shin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


/**
* @file		uts_mmf_session_common.h	
* @brief	This is a suite of unit test cases for Session APIs.
* @version	Initial Creation V0.1
* @date		2010.07.06
*/


#ifndef UTS_MM_FRAMEWORK_SESSION_COMMON_H
#define UTS_MM_FRAMEWORK_SESSION_COMMON_H


#include <mm_session.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <stdio.h>
#include <string.h>
#include <tet_api.h>
#include <unistd.h>
#include <glib.h>


void startup();
void cleanup();

void (*tet_startup)() = startup;
void (*tet_cleanup)() = cleanup;


void utc_mm_session_init_func_01 ();
void utc_mm_session_init_func_02 ();
void utc_mm_session_init_func_03 ();
void utc_mm_session_init_func_04 ();

void utc_mm_session_finish_func_01 ();
void utc_mm_session_finish_func_02 ();


void utc_mm_session_init_ex_func_01();
void utc_mm_session_init_ex_func_02();


#endif /* UTS_MM_FRAMEWORK_SESSION_COMMON_H */
