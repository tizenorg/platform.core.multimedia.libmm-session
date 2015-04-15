/*
 * libmm-session
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungbae Shin <seungbae.shin at samsung.com>, Sangchul Lee <sc11.lee at samsung.com>
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

typedef enum {
	MM_SUBSESSION_TYPE_VOICE = 0,
	MM_SUBSESSION_TYPE_RINGTONE,
	MM_SUBSESSION_TYPE_MEDIA,
	MM_SUBSESSION_TYPE_INIT,
	MM_SUBSESSION_TYPE_VR_NORMAL,
	MM_SUBSESSION_TYPE_VR_DRIVE,
	MM_SUBSESSION_TYPE_RECORD_STEREO,
	MM_SUBSESSION_TYPE_RECORD_MONO,
	MM_SUBSESSION_TYPE_NUM
} mm_subsession_t;

typedef enum {
	MM_SUBSESSION_OPTION_NONE = 0,
	MM_SUBSESSION_OPTION_NUM
	/* NOTE : Do not exceed 15, because of using mm_subsession_option_priv_t type with it internally */
} mm_subsession_option_t;

typedef enum {
	MM_SESSION_SUB_TYPE_NONE = 0,
	MM_SESSION_SUB_TYPE_SHARE,
	MM_SESSION_SUB_TYPE_EXCLUSIVE
} mm_session_sub_t;


/**
 * This function delete session information to system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * @see		_mm_session_util_write_type _mm_session_util_read_type _mm_session_util_write_information _mm_session_util_read_information
 * @since
 */
int _mm_session_util_delete_information(int app_pid);


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
 * @see		_mm_session_util_delete_information _mm_session_util_read_type
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
 * @see		_mm_session_util_write_type _mm_session_util_delete_information
 * @since
 */
int _mm_session_util_read_type(int app_pid, int *sessiontype);


/**
 * This function write session information to system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @param	session_type	[in] Multimedia Session type
 * @param	flags	[in] Multimedia Session options
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type and Session option are unique for each application.
 * @see		_mm_session_util_delete_information _mm_session_util_read_information
 * @since
 */
int _mm_session_util_write_information(int app_pid, int session_type, int flags);


/**
 * This function read session information from system
 *
 * @param	app_pid [in] Application pid (if -1, use caller process)
 * @param	sessiontype	[out] Multimedia Session type
 * @param	flags	[out] Multimedia Session options
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Session type is unique for each application.
 * @see		_mm_session_util_write_information _mm_session_util_delete_information
 * @since
 */
int _mm_session_util_read_information(int app_pid, int *session_type, int *flags);


/**
 * This function set sub-session type
 *
 * @param	subsession [in] subsession type
 * @param	option	[in] option of subsession type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * @see		mm_session_get_subsession
 * @since
 */
int mm_session_set_subsession (mm_subsession_t subsession, mm_subsession_option_t option);

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

/**
 * This function set sub-event type
 *
 * @param	subevent [in] subevent type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * @see		mm_session_get_subevent
 * @since
 */
int mm_session_set_subevent (mm_session_sub_t subevent);

/**
 * This function get current sub-event type
 *
 * @param	subevent [out] subevent type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	This function is only for internal implementation do not use this at application
 * 			Session type is unique for each application.
 * @see		mm_session_set_subsevnt
 * @since
 */
int mm_session_get_subevent (mm_session_sub_t *subevent);

#ifdef __cplusplus
}
#endif

#endif
