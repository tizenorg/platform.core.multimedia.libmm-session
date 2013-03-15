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
 * This file declares common data structure of multimedia framework.
 *
 * @file		mm_session_private.h
 * @author
 * @version		1.0
 * @brief		This file declares multimedia framework session type.
 */
#ifndef	_MM_SESSION_PRIVATE_H_
#define	_MM_SESSION_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <mm_session.h>

/**
  * This enumeration defines session types for internal usage.
  */
enum MMSessionTypePrivate{
	MM_SESSION_TYPE_ALARM = 2,
	MM_SESSION_TYPE_EMERGENCY,
	MM_SESSION_TYPE_NOTIFY,
	MM_SESSION_TYPE_CALL,
	MM_SESSION_TYPE_VIDEOCALL,
	MM_SESSION_TYPE_RICH_CALL,
	MM_SESSION_PRIVATE_TYPE_NUM
};

typedef enum {
	MM_SUBSESSION_TYPE_VOICE = 0,
	MM_SUBSESSION_TYPE_RINGTONE,
	MM_SUBSESSION_TYPE_MEDIA
} mm_subsession_t;

/**
 * This function delete session type information to system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * 			if application want to change session type, Finish session first and Init again
 * @see		_mm_session_util_write_type _mm_session_util_read_type
 * @since
 */
int _mm_session_util_delete_type(int app_pid);



/**
 * This function write session type information to system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @param	sessiontype	[in] Multimedia Session type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * 			if application want to change session type, Finish session first and Init again
 * @see		_mm_session_util_delete_type _mm_session_util_read_type
 * @since
 */
int _mm_session_util_write_type(int app_pid, int sessiontype);


/**
 * This function read session type information from system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @param	sessiontype	[out] Multimedia Session type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Session type is unique for each application.
 * 			if application want to change session type, Finish session first and Init again
 * @see		_mm_session_util_write_type _mm_session_util_delete_type
 * @since
 */
int _mm_session_util_read_type(int app_pid, int *sessiontype);

/**
 * This function set sub-session type
 *
 * @param	subsession [in] subsession type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * @see		mm_session_get_subsession
 * @since
 */
int mm_session_set_subsession (mm_subsession_t subsession);

/**
 * This function get current sub-session type
 *
 * @param	subsession [out] subsession type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * @see		mm_session_set_subsession
 * @since
 */
int mm_session_get_subsession (mm_subsession_t *subsession);

#ifdef __cplusplus
}
#endif

#endif
