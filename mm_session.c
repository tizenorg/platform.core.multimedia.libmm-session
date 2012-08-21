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


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mm_session_private.h>
#include <mm_error.h>
#include <errno.h>
#include <audio-session-manager.h>
#include <dlog.h>

#include <glib.h>

#define EXPORT_API __attribute__((__visibility__("default")))
#define MAX_FILE_LENGTH 256
#define LOG_TAG	"MMFW_SESSION"
#define debug_log(fmt, arg...) SLOG(LOG_VERBOSE, LOG_TAG, "[%s:%d] "fmt"\n", __FUNCTION__,__LINE__,##arg)
#define debug_warning(fmt, arg...) SLOG(LOG_WARN, LOG_TAG, "[%s:%d] "fmt"\n", __FUNCTION__,__LINE__,##arg)
#define debug_error(fmt, arg...) SLOG(LOG_ERROR, LOG_TAG, "[%s:%d] "fmt"\n", __FUNCTION__,__LINE__,##arg)

typedef struct {
	session_callback_fn fn;
	void* data;
	session_msg_t msg;
	session_event_t event;
}session_monitor_t;

int g_call_asm_handle = -1;
int g_monitor_asm_handle = -1;
session_monitor_t g_monitor_data;

ASM_cb_result_t asm_monitor_callback(int handle, ASM_event_sources_t event_src, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data);

EXPORT_API
int mm_session_init(int sessiontype)
{
	debug_log("");
	return mm_session_init_ex(sessiontype, NULL, NULL);
}

EXPORT_API
int mm_session_init_ex(int sessiontype, session_callback_fn callback, void* user_param)
{
	int error = 0;
	int result = MM_ERROR_NONE;
	int ltype = 0;

	debug_log("type : %d", sessiontype);

	if(sessiontype != MM_SESSION_TYPE_SHARE && sessiontype != MM_SESSION_TYPE_EXCLUSIVE &&
			sessiontype != MM_SESSION_TYPE_NOTIFY && sessiontype != MM_SESSION_TYPE_CALL && sessiontype != MM_SESSION_TYPE_ALARM  && sessiontype != MM_SESSION_TYPE_VIDEOCALL
			&& sessiontype != MM_SESSION_TYPE_RICH_CALL) {
		debug_error("Invalid argument %d",sessiontype);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	result = _mm_session_util_read_type(-1, &ltype);
	if(MM_ERROR_INVALID_HANDLE != result) {
		debug_error("Session already initialized. Please finish current session first");
		return MM_ERROR_POLICY_DUPLICATED;
	}

	if(sessiontype == MM_SESSION_TYPE_CALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_CALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_VIDEOCALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_CAMERA|ASM_RESOURCE_VIDEO_OVERLAY, &error))	{
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
		if(!ASM_register_sound(-1, &g_call_asm_handle, ASM_EVENT_RICH_CALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			debug_error("Can not register sound");
			return MM_ERROR_INVALID_HANDLE;
		}
	} else {
		if(NULL == callback) {
			debug_warning("Null callback function");
		} else {
			g_monitor_data.fn = callback;
			g_monitor_data.data = user_param;
			if(!ASM_register_sound(-1, &g_monitor_asm_handle, ASM_EVENT_MONITOR, ASM_STATE_NONE, asm_monitor_callback, (void*)&g_monitor_data, ASM_RESOURCE_NONE, &error)) {
				debug_error("Can not register monitor");
				return MM_ERROR_INVALID_HANDLE;
			}
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
			ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error);
		}
		return result;
	}

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_finish()
{
	int error = 0;
	int result = MM_ERROR_NONE;
	int sessiontype = MM_SESSION_TYPE_SHARE;
	ASM_sound_states_t state = ASM_STATE_NONE;
	debug_log("");

	if(g_call_asm_handle == -1) {
		if(g_monitor_asm_handle == -1) {
			//register monitor handle to get MSL status of caller process
			if(!ASM_register_sound(-1, &g_monitor_asm_handle, ASM_EVENT_MONITOR, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
				debug_error("[%s] Can not register monitor", __func__);
				return MM_ERROR_INVALID_HANDLE;
			}
		}

		if(!ASM_get_process_session_state(g_monitor_asm_handle, &state, &error)) {
			debug_error("[%s] Can not get process status", __func__);
			return MM_ERROR_POLICY_INTERNAL;
		} else {
			switch(state)
			{
			case ASM_STATE_IGNORE:
			case ASM_STATE_NONE:
				break;
			case ASM_STATE_PLAYING:
			case ASM_STATE_WAITING:
			case ASM_STATE_STOP:
			case ASM_STATE_PAUSE:
			case ASM_STATE_PAUSE_BY_APP:
				debug_error("[%s] MSL instance still alive", __func__);
				return MM_ERROR_POLICY_BLOCKED;
			}
		}
	}


	result = _mm_session_util_read_type(-1, &sessiontype);
	if(MM_ERROR_NONE != result) {
		debug_error("Can not read current type");
		return result;
	}

	if(sessiontype == MM_SESSION_TYPE_CALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_CALL, &error)) {
			debug_error("\"CALL\" ASM unregister failed");
			return MM_ERROR_INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_VIDEOCALL, &error)) {
			debug_error("\"VIDEOCALL\" ASM unregister failed");
			return MM_ERROR_INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
		if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_RICH_CALL, &error)) {
			debug_error("\"RICH-CALL\" ASM unregister failed");
			return MM_ERROR_INVALID_HANDLE;
		}
		g_call_asm_handle = -1;
	} else {
		if(g_monitor_asm_handle != -1) { //TODO :: this is trivial check. this should be removed later.
			if(!ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error)) {
				debug_error("ASM unregister failed");
				return MM_ERROR_INVALID_HANDLE;
			}
			g_monitor_asm_handle = -1;
		}
	}

	result = _mm_session_util_delete_type(-1);
	if(result != MM_ERROR_NONE)
		return result;

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_set_subsession (mm_subsession_t subsession)
{
	int error = 0;
	int result = MM_ERROR_NONE;
	debug_log("");

	if(g_call_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}

	/* FIXME : Error handling */
	ASM_set_subsession (g_call_asm_handle, subsession, &error, NULL);

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_get_subsession (mm_subsession_t *subsession)
{
	int error = 0;
	int result = MM_ERROR_NONE;
	int sessiontype = MM_SESSION_TYPE_SHARE;
	ASM_sound_states_t state = ASM_STATE_NONE;
	debug_log("");

	if(g_call_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}

	ASM_get_subsession (g_call_asm_handle, subsession, &error, NULL);

	debug_log("ASM_get_subsession returned [%d]\n", *subsession);

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
	EXPORT_API
	int mm_session_finish() {
		int error = 0;
		int result = MM_ERROR_NONE;
		int sessiontype = MM_SESSION_TYPE_SHARE;

		result = _mm_session_util_read_type(-1, &sessiontype);
		if(MM_ERROR_NONE != result) {
			return result;
		}

		if(sessiontype == MM_SESSION_TYPE_CALL) {
			if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_CALL, &error)) {
				return MM_ERROR_INVALID_HANDLE;
			}
		} else if(sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
			if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_VIDEOCALL, &error)) {
				return MM_ERROR_INVALID_HANDLE;
			}
		} else if(sessiontype == MM_SESSION_TYPE_RICH_CALL) {
			if(!ASM_unregister_sound(g_call_asm_handle, ASM_EVENT_RICH_CALL, &error)) {
				return MM_ERROR_INVALID_HANDLE;
			}
		}

		result = _mm_session_util_delete_type(-1);
		if(result != MM_ERROR_NONE)
			return result;

		return MM_ERROR_NONE;
	}


	return MM_ERROR_NONE;
}

EXPORT_API
int _mm_session_util_write_type(int app_pid, int sessiontype)
{
	pid_t mypid;
	int fd = -1;
	char filename[MAX_FILE_LENGTH];
	int res=0;

	if(sessiontype != MM_SESSION_TYPE_SHARE && sessiontype != MM_SESSION_TYPE_EXCLUSIVE &&
			sessiontype != MM_SESSION_TYPE_NOTIFY && sessiontype != MM_SESSION_TYPE_CALL && sessiontype != MM_SESSION_TYPE_ALARM
			&& sessiontype != MM_SESSION_TYPE_RICH_CALL) {
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
	case ASM_EVENT_SOURCE_CALL_END:
		return MM_SESSION_EVENT_CALL;

	case ASM_EVENT_SOURCE_EARJACK_UNPLUG:
		return MM_SESSION_EVENT_EARJACK_UNPLUG;

	case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
		return MM_SESSION_EVENT_RESOURCE_CONFLICT;

	case ASM_EVENT_SOURCE_ALARM_START:
	case ASM_EVENT_SOURCE_ALARM_END:
		return MM_SESSION_EVENT_ALARM;

	case ASM_EVENT_SOURCE_OTHER_APP:
	case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
	default:
		return MM_SESSION_EVENT_OTHER_APP;
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
	_mm_session_util_delete_type(-1);
}

__attribute__ ((constructor))
void __mmsession_initialize(void)
{

}

