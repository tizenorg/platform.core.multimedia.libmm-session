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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mm_session_private.h>
#include <mm_error.h>
#include <mm_debug.h>
#include <errno.h>
#include <glib.h>
#include <pthread.h>

#define EXPORT_API __attribute__((__visibility__("default")))
#define MAX_FILE_LENGTH 256

int g_session_type = -1;

EXPORT_API
int mm_session_init(int sessiontype)
{
	debug_fenter();
	return mm_session_init_ex(sessiontype, NULL, NULL);
	debug_fleave();
}

EXPORT_API
int mm_session_init_ex(int sessiontype, session_callback_fn callback, void* user_param)
{
	int result = MM_ERROR_NONE;
	int ltype = 0;
	bool do_not_update_session_info = false;

	debug_fenter();
	debug_log("type : %d", sessiontype);

	if (sessiontype < MM_SESSION_TYPE_MEDIA || sessiontype >= MM_SESSION_TYPE_NUM) {
		debug_error("Invalid argument %d",sessiontype);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_type(-1, &ltype);
	if (MM_ERROR_INVALID_HANDLE != result) {
		if ((ltype == MM_SESSION_TYPE_MEDIA_RECORD) && sessiontype == MM_SESSION_TYPE_MEDIA) {
			/* already set by mm-camcorder, mm-sound(pcm in), keep going */
			do_not_update_session_info = true;
		} else {
			debug_error("Session already initialized. Please finish current session first");
			return MM_ERROR_POLICY_DUPLICATED;
		}
	}

	/* Monitor Callback */
	if (NULL == callback) {
		debug_warning("Null callback function");
	} else {
		debug_warning("It was deprecated, do not use monitor callback, callback(%p), user_param(%p)", callback, user_param);
	}

	g_session_type = sessiontype;

	if (!do_not_update_session_info) {
		result = _mm_session_util_write_type(-1, sessiontype);
		if (MM_ERROR_NONE != result) {
			debug_error("Write type failed");
			g_session_type = -1;
			return result;
		}
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_update_option(session_update_type_t update_type, int options)
{
	int result = MM_ERROR_NONE;
	int ltype = 0;
	int loption = 0;

	debug_log("update_type: %d(0:Add, 1:Remove), options: %x", update_type, options);

	if (update_type < 0 || update_type >= MM_SESSION_UPDATE_TYPE_NUM) {
		debug_error("Invalid update_type value(%d)", update_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if (options < 0) {
		debug_error("Invalid options value(%x)", options);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_information(-1, &ltype, &loption);
	if (result) {
		debug_error("failed to _mm_session_util_read_information(), ret(%x)", result);
		return result;
	}
	debug_log("[current] session_type: %d, session_option: %x", ltype, loption);

	if (update_type == MM_SESSION_UPDATE_TYPE_ADD) {
		loption |= options;
	} else if (update_type == MM_SESSION_UPDATE_TYPE_REMOVE) {
		loption &= ~options;
	}

	result = _mm_session_util_write_information(-1, ltype, loption);
	if (result) {
		debug_error("failed to _mm_session_util_write_information(), ret(%x)", result);
		return result;
	}

	debug_log("[updated] session_type: %d, session_option: %x", ltype, loption);


	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_add_watch_callback(int watchevent, int watchstate, watch_callback_fn callback, void* user_param)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_get_current_type(int *sessiontype)
{
	int result = MM_ERROR_NONE;
	int ltype = 0;

	debug_fenter();

	if (sessiontype == NULL) {
		debug_error("input argument is NULL\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_type(-1, &ltype);
	if (result == MM_ERROR_NONE) {
		debug_log("Current process session type = [%d]\n", ltype);
		*sessiontype = ltype;
	} else {
		debug_error("failed to get current process session type!!\n");
	}

	debug_fleave();

	return result;
}

EXPORT_API
int mm_session_get_current_information(int *session_type, int *session_options)
{
	int result = MM_ERROR_NONE;
	int ltype = 0;
	int loption = 0;

	debug_fenter();

	if (session_type == NULL) {
		debug_error("input argument is NULL\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_information(-1, &ltype, &loption);
	if (result == MM_ERROR_NONE) {
		debug_log("Current process session type = [%d], options = [%x]\n", ltype, loption);
		*session_type = ltype;
		*session_options = loption;
	} else {
		debug_error("failed to get current process session type, option!!\n");
	}

	debug_fleave();

	return result;
}

EXPORT_API
int mm_session_finish(void)
{
	int result = MM_ERROR_NONE;
	int sessiontype = MM_SESSION_TYPE_MEDIA;

	debug_fenter();

	result = _mm_session_util_read_type(-1, &sessiontype);
	if (MM_ERROR_NONE != result) {
		debug_error("Can not read current type");
		return result;
	}

	/* Check monitor handle */
	result = _mm_session_util_delete_information(-1);
	if(result != MM_ERROR_NONE)
		return result;

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_remove_watch_callback(int watchevent, int watchstate)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_set_subsession(mm_subsession_t subsession, mm_subsession_option_t option)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_get_subsession(mm_subsession_t *subsession)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_set_subevent(mm_session_sub_t subevent)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_get_subevent(mm_session_sub_t *subevent)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int mm_session_reset_resumption_info(void)
{
	debug_fenter();

	debug_fleave();

	return MM_ERROR_NOT_SUPPORT_API;
}

EXPORT_API
int _mm_session_util_delete_information(int app_pid)
{
	pid_t mypid;
	char filename[MAX_FILE_LENGTH];

	if(app_pid == -1)
		mypid = getpid();
	else
		mypid = (pid_t)app_pid;

	////// DELETE SESSION TYPE /////////
	snprintf(filename, sizeof(filename)-1, "/tmp/mm_session_%d",mypid);
	if(-1 ==  unlink(filename))
		return MM_ERROR_FILE_NOT_FOUND;
	////// DELETE SESSION TYPE /////////

	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_write_type(int app_pid, int sessiontype)
{
	pid_t mypid;
	int fd = -1;
	char filename[MAX_FILE_LENGTH];

	if(sessiontype < MM_SESSION_TYPE_MEDIA || sessiontype >= MM_SESSION_TYPE_NUM) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(app_pid == -1)
		mypid = getpid();
	else
		mypid = (pid_t)app_pid;

	////// WRITE SESSION TYPE /////////
	snprintf(filename, sizeof(filename)-1, "/tmp/mm_session_%d",mypid);
	fd = open(filename, O_WRONLY | O_CREAT, 0644 );
	if(fd < 0) {
		debug_error("open() failed with %d",errno);
		return MM_ERROR_FILE_WRITE;
	}
	sessiontype = sessiontype << 16;
	write(fd, &sessiontype, sizeof(int));
	if(0 > fchmod (fd, 00777)) {
		debug_error("fchmod failed with %d", errno);
	} else {
		debug_warning("write sessiontype(%d) to /tmp/mm_session_%d", sessiontype >> 16, mypid);
	}
	close(fd);
	////// WRITE SESSION TYPE /////////

	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_read_type(int app_pid, int *sessiontype)
{
	pid_t mypid;
	int fd = -1;
	char filename[MAX_FILE_LENGTH];

	debug_fenter();

	if(sessiontype == NULL)
		return MM_ERROR_INVALID_ARGUMENT;

	if(app_pid == -1)
		mypid = getpid();
	else
		mypid = (pid_t)app_pid;

	////// READ SESSION TYPE /////////
	snprintf(filename, sizeof(filename)-1, "/tmp/mm_session_%d",mypid);
	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		return MM_ERROR_INVALID_HANDLE;
	}
	read(fd, sessiontype, sizeof(int));
	*sessiontype = *sessiontype >> 16;
	debug_warning("read sessiontype(%d) from /tmp/mm_session_%d", *sessiontype, mypid);
	close(fd);
	////// READ SESSION TYPE /////////

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_write_information(int app_pid, int session_type, int flags)
{
	pid_t mypid;
	int fd = -1;
	char filename[MAX_FILE_LENGTH];
	int result_info = 0;

	if(session_type < MM_SESSION_TYPE_MEDIA || session_type >= MM_SESSION_TYPE_NUM) {
		return MM_ERROR_INVALID_ARGUMENT;
	}
	if(flags < 0) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(app_pid == -1) {
		mypid = getpid();
	} else {
		mypid = (pid_t)app_pid;
	}

	////// WRITE SESSION INFO /////////
	snprintf(filename, sizeof(filename)-1, "/tmp/mm_session_%d",mypid);
	fd = open(filename, O_WRONLY | O_CREAT, 0644 );
	if(fd < 0) {
		debug_error("open() failed with %d",errno);
		return MM_ERROR_FILE_WRITE;
	}

	result_info = (flags) | (session_type << 16);
	write(fd, &result_info, sizeof(int));
	if(0 > fchmod (fd, 00777)) {
		debug_error("fchmod failed with %d", errno);
	} else {
		debug_warning("write session information(%x) to /tmp/mm_session_%d", result_info, mypid);
	}
	close(fd);
	////// WRITE SESSION INFO /////////

	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_read_information(int app_pid, int *session_type, int *flags)
{
	pid_t mypid;
	int fd = -1;
	char filename[MAX_FILE_LENGTH];
	int result_info = 0;

	debug_fenter();

	if(session_type == NULL || flags == NULL) {
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(app_pid == -1) {
		mypid = getpid();
	} else {
		mypid = (pid_t)app_pid;
	}

	////// READ SESSION INFO /////////
	snprintf(filename, sizeof(filename)-1, "/tmp/mm_session_%d",mypid);
	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		return MM_ERROR_INVALID_HANDLE;
	}
	read(fd, &result_info, sizeof(int));
	*session_type = result_info >> 16;
	*flags = result_info & 0x0000ffff;

	debug_warning("read session_type(%d), session_option(%x) from /tmp/mm_session_%d", *session_type, *flags, mypid);
	close(fd);
	////// READ SESSION INFO /////////

	debug_fleave();

	return MM_ERROR_NONE;
}

__attribute__ ((destructor))
void __mmsession_finalize(void)
{
	debug_fenter();

	_mm_session_util_delete_information(-1);

	debug_fleave();
}

__attribute__ ((constructor))
void __mmsession_initialize(void)
{

}

