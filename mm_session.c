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
#include <audio-session-manager.h>
#include <glib.h>
#include <pthread.h>

#define EXPORT_API __attribute__((__visibility__("default")))
#define MAX_FILE_LENGTH 256

#define TRY_LOCK(x, x_ret) \
do {\
    x_ret = pthread_mutex_trylock(&(x));\
    if(x_ret != 0) {\
        debug_warning("Mutex trylock failed, (0x%x)",x_ret);\
    }\
} while(0)
#define LOCK(x) \
do {\
    if(pthread_mutex_lock(&(x)) != 0) {\
        debug_error("Mutex lock error");\
    }\
} while(0)
#define UNLOCK(x) \
do {\
    if(pthread_mutex_unlock(&(x)) != 0) {\
        debug_error("Mutex unlock error");\
    }\
} while(0)
#define DESTROY(x) \
do {\
    if(pthread_mutex_destroy(&(x)) != 0) {\
        debug_error("Mutex destroy error");\
    }\
} while(0)

typedef struct {
	session_callback_fn fn;
	void* data;
	session_msg_t msg;
	session_event_t event;
}session_monitor_t;

int g_call_asm_handle = -1;
int g_monitor_asm_handle = -1;
session_monitor_t g_monitor_data;

pthread_mutex_t g_mutex_monitor = PTHREAD_MUTEX_INITIALIZER;

ASM_cb_result_t asm_monitor_callback(int handle, ASM_event_sources_t event_src, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data);

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
	int error = 0;
	int result = MM_ERROR_NONE;
	int ltype = 0;
	pthread_mutex_init(&g_mutex_monitor, NULL);
	debug_fenter();
	debug_log("type : %d", sessiontype);

	if(sessiontype < MM_SESSION_TYPE_SHARE || sessiontype >= MM_SESSION_PRIVATE_TYPE_NUM) {
		debug_error("Invalid argument %d",sessiontype);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_type(-1, &ltype);
	if(MM_ERROR_INVALID_HANDLE != result) {
		debug_error("Session already initialized. Please finish current session first");
		return MM_ERROR_POLICY_DUPLICATED;
	}

	/* Monitor Callback */
	if(NULL == callback) {
		debug_warning("Null callback function");
	} else {
		g_monitor_data.fn = callback;
		g_monitor_data.data = user_param;
		LOCK(g_mutex_monitor);
		if(!ASM_register_sound(-1, &g_monitor_asm_handle, ASM_EVENT_MONITOR, ASM_STATE_NONE, asm_monitor_callback, (void*)&g_monitor_data, ASM_RESOURCE_NONE, &error)) {
			debug_error("Can not register monitor");
			UNLOCK(g_mutex_monitor);
			return MM_ERROR_INVALID_HANDLE;
		}
		UNLOCK(g_mutex_monitor);
	}

	/* Register here for call session types */
	if(sessiontype == MM_SESSION_TYPE_CALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_CALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_VIDEOCALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_CAMERA|ASM_RESOURCE_VIDEO_OVERLAY, &error)) {
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_RICH_CALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	}

	result = _mm_session_util_write_type(-1, sessiontype);
	if(MM_ERROR_NONE != result) {
		debug_error("Write type failed");
		if(sessiontype == MM_SESSION_TYPE_CALL) {
			ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_CALL, &error);
		} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
			ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_VIDEOCALL, &error);
		} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
			ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_RICH_CALL, &error);
		} else {
			LOCK(g_mutex_monitor);
			ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error);
			UNLOCK(g_mutex_monitor);
		}
		return result;
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_get_current_type (int *sessiontype)
{
	int result = MM_ERROR_NONE;
	int ltype = 0;

	if (sessiontype == NULL) {
		debug_error("input argument is NULL\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_type(-1, &ltype);
	if(result == MM_ERROR_NONE) {
		debug_log("Current process session type = [%d]\n", ltype);
		*sessiontype = ltype;
	} else {
		debug_error("failed to get current process session type!!\n");
	}

	return result;
}

EXPORT_API
int mm_session_finish()
{
	int error = 0;
	int result = MM_ERROR_NONE;
	int sessiontype = MM_SESSION_TYPE_SHARE;
	ASM_sound_states_t state = ASM_STATE_NONE;

	debug_fenter();

	result = _mm_session_util_read_type(-1, &sessiontype);
	if(MM_ERROR_NONE != result) {
		debug_error("Can not read current type");
		DESTROY(g_mutex_monitor);
		return result;
	}

	/* Unregister call session here */
	if(sessiontype == MM_SESSION_TYPE_CALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_CALL, &error)) {
			debug_error("\"CALL\" ASM unregister failed");
			goto INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_VIDEOCALL, &error)) {
			debug_error("\"VIDEOCALL\" ASM unregister failed");
			goto INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_RICH_CALL, &error)) {
			debug_error("\"RICH-CALL\" ASM unregister failed");
			goto INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	}

	/* Check monitor handle */
	TRY_LOCK(g_mutex_monitor, error);
	if (!error) {
		if(g_monitor_asm_handle != -1) {
			if(!ASM_get_process_session_state(g_monitor_asm_handle, &state, &error)) {
				debug_error("[%s] Can not get process status", __func__);
				UNLOCK(g_mutex_monitor);
				DESTROY(g_mutex_monitor);
				return MM_ERROR_POLICY_INTERNAL;
			} else {
				switch(state) {
				case ASM_STATE_IGNORE:
				case ASM_STATE_NONE:
					break;
				case ASM_STATE_PLAYING:
				case ASM_STATE_WAITING:
				case ASM_STATE_STOP:
				case ASM_STATE_PAUSE:
				case ASM_STATE_PAUSE_BY_APP:
					debug_error("[%s] MSL instance still alive", __func__);
					UNLOCK(g_mutex_monitor);
					DESTROY(g_mutex_monitor);
					return MM_ERROR_POLICY_BLOCKED;
				}
			}
			/* Unregister monitor */
			if(!ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error)) {
				debug_error("ASM unregister monitor failed");
				UNLOCK(g_mutex_monitor);
				goto INVALID_HANDLE;
			} else {
				debug_log("ASM unregister monitor success");
				g_monitor_asm_handle = -1;
			}
		}
		UNLOCK(g_mutex_monitor);
		DESTROY(g_mutex_monitor);
	}

	result = _mm_session_util_delete_type(-1);
	if(result != MM_ERROR_NONE)
		return result;

	debug_fleave();

	return MM_ERROR_NONE;

INVALID_HANDLE:
	DESTROY(g_mutex_monitor);
	return MM_ERROR_INVALID_HANDLE;
}

EXPORT_API
int mm_session_set_subsession (mm_subsession_t subsession)
{
	int error = 0;
	int result = MM_ERROR_NONE;

	debug_fenter();

	if(g_call_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}

	/* FIXME : Error handling */
	ASM_set_subsession (g_call_asm_handle, subsession, &error, NULL);

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_get_subsession (mm_subsession_t *subsession)
{
	int error = 0;
	int result = MM_ERROR_NONE;
	debug_fenter();

	if(g_call_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}

	ASM_get_subsession (g_call_asm_handle, subsession, &error, NULL);

	debug_log("ASM_get_subsession returned [%d]\n", *subsession);
	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_delete_type(int app_pid)
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

	if(sessiontype < MM_SESSION_TYPE_SHARE || sessiontype >= MM_SESSION_PRIVATE_TYPE_NUM) {
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
	write(fd, &sessiontype, sizeof(int));
	if(0 > fchmod (fd, 00777)) {
		debug_log("fchmod failed with %d", errno);
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
	close(fd);
	////// READ SESSION TYPE /////////

	return MM_ERROR_NONE;
}

gboolean _asm_monitor_cb(gpointer *data)
{
	session_monitor_t* monitor = (session_monitor_t*)data;
	if (monitor) {
		if (monitor->fn) {
			debug_log("calling _asm_monitor_cb()");
			monitor->fn(monitor->msg, monitor->event, monitor->data);
		}
	}

	return FALSE;
}

static session_event_t _translate_from_asm_to_mm_session (ASM_event_sources_t event_src)
{
	switch (event_src)
	{
	case ASM_EVENT_SOURCE_CALL_START:
		return MM_SESSION_EVENT_CALL;

	case ASM_EVENT_SOURCE_EARJACK_UNPLUG:
		return MM_SESSION_EVENT_EARJACK_UNPLUG;

	case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
		return MM_SESSION_EVENT_RESOURCE_CONFLICT;

	case ASM_EVENT_SOURCE_ALARM_START:
	case ASM_EVENT_SOURCE_ALARM_END:
		return MM_SESSION_EVENT_ALARM;

	case ASM_EVENT_SOURCE_EMERGENCY_START:
	case ASM_EVENT_SOURCE_EMERGENCY_END:
		return MM_SESSION_EVENT_EMERGENCY;

	case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
		return MM_SESSION_EVENT_RESUMABLE_MEDIA;

	case ASM_EVENT_SOURCE_MEDIA:
	default:
		return MM_SESSION_EVENT_MEDIA;
	}
}

ASM_cb_result_t
asm_monitor_callback(int handle, ASM_event_sources_t event_src, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data)
{
	ASM_cb_result_t	cb_res = ASM_CB_RES_NONE;
	session_monitor_t *monitor = (session_monitor_t*)cb_data;

	debug_log("monitor callback called for handle %d, event_src %d", handle, event_src);
	if(!monitor) {
		debug_log("monitor instance is null\n");
		return ASM_CB_RES_IGNORE;
	}

	switch(command)
	{
	case ASM_COMMAND_STOP:
	case ASM_COMMAND_PAUSE:
		//call session_callback_fn for stop here
		if(monitor->fn) {
			monitor->msg = MM_SESSION_MSG_STOP;
			monitor->event = _translate_from_asm_to_mm_session (event_src);
			g_idle_add((GSourceFunc)_asm_monitor_cb, (gpointer)monitor);
		}
		cb_res = (command == ASM_COMMAND_STOP)? ASM_CB_RES_STOP : ASM_CB_RES_PAUSE;
		break;

	case ASM_COMMAND_RESUME:
	case ASM_COMMAND_PLAY:
		//call session_callback_fn for resume here
		if(monitor->fn) {
			monitor->msg = MM_SESSION_MSG_RESUME;
			monitor->event = _translate_from_asm_to_mm_session (event_src);
			g_idle_add((GSourceFunc)_asm_monitor_cb, (gpointer)monitor);
		}
		cb_res = ASM_CB_RES_IGNORE;
		break;

	default:
		break;
	}
	return cb_res;
}

__attribute__ ((destructor))
void __mmsession_finalize(void)
{
	int error=0;

	debug_fenter();

	TRY_LOCK(g_mutex_monitor, error);
	if (!error) {
		if(g_monitor_asm_handle != -1) {
			/* Unregister monitor */
			if(!ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error)) {
				debug_error("ASM unregister monitor failed");
			} else {
				debug_log("ASM unregister monitor success");
				g_monitor_asm_handle = -1;
			}
		}
		UNLOCK(g_mutex_monitor);
		DESTROY(g_mutex_monitor);
	}
	_mm_session_util_delete_type(-1);

	debug_fleave();
}

__attribute__ ((constructor))
void __mmsession_initialize(void)
{

}

