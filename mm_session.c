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

typedef struct {
	watch_callback_fn fn;
	void* data;
	session_watch_event_t event;
	session_watch_state_t state;
}session_watch_t;

int g_asm_handle = -1;
int g_session_type = -1;
int g_monitor_asm_handle = -1;
session_monitor_t g_monitor_data;
session_watch_t g_watch_data;

pthread_mutex_t g_mutex_monitor = PTHREAD_MUTEX_INITIALIZER;

ASM_cb_result_t asm_monitor_callback(int handle, ASM_event_sources_t event_src, ASM_sound_commands_t command, unsigned int sound_status, void* cb_data);
ASM_cb_result_t asm_watch_callback(int handle, ASM_sound_events_t sound_event, ASM_sound_states_t sound_state, void* cb_data);
static session_event_t _translate_from_event_src_to_mm_session(ASM_event_sources_t event_src);
static session_watch_event_t _translate_from_asm_event_to_mm_session(ASM_sound_events_t sound_event);
static ASM_sound_events_t _translate_from_mm_session_to_asm_event(session_watch_event_t watch_event);
static ASM_sound_states_t _translate_from_mm_session_to_asm_state(session_watch_state_t watch_state);


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
	bool do_not_update_session_info = false;
	pthread_mutex_init(&g_mutex_monitor, NULL);
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
		if (sessiontype != MM_SESSION_TYPE_RECORD_AUDIO &&
			sessiontype != MM_SESSION_TYPE_RECORD_VIDEO ) {
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
	}

	/* Register here for call session types */
	if (sessiontype == MM_SESSION_TYPE_CALL) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_CALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			goto REGISTER_FAILURE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_VIDEOCALL, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_CAMERA|ASM_RESOURCE_VIDEO_OVERLAY, &error)) {
			goto REGISTER_FAILURE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_VOIP) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_VOIP, ASM_STATE_PLAYING, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			goto REGISTER_FAILURE;
		}
	}
	/* Register here for advanced session types (using asm_sub_event) */
	  else if (sessiontype == MM_SESSION_TYPE_VOICE_RECOGNITION) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_VOICE_RECOGNITION, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			goto REGISTER_FAILURE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_RECORD_AUDIO) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_MMCAMCORDER_AUDIO, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_NONE, &error)) {
			goto REGISTER_FAILURE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_RECORD_VIDEO) {
		if(!ASM_register_sound(-1, &g_asm_handle, ASM_EVENT_MMCAMCORDER_VIDEO, ASM_STATE_NONE, NULL, NULL, ASM_RESOURCE_CAMERA|ASM_RESOURCE_VIDEO_OVERLAY|ASM_RESOURCE_HW_ENCODER, &error)) {
			goto REGISTER_FAILURE;
		}
	}

	g_session_type = sessiontype;

	if (!do_not_update_session_info) {
		result = _mm_session_util_write_type(-1, sessiontype);
		if (MM_ERROR_NONE != result) {
			debug_error("Write type failed");
			if (sessiontype == MM_SESSION_TYPE_CALL) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_CALL, &error);
			} else if (sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_VIDEOCALL, &error);
			} else if (sessiontype == MM_SESSION_TYPE_VOIP) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_VOIP, &error);
			} else if (sessiontype == MM_SESSION_TYPE_VOICE_RECOGNITION) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_VOICE_RECOGNITION, &error);
			} else if (sessiontype == MM_SESSION_TYPE_RECORD_AUDIO) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_MMCAMCORDER_AUDIO, &error);
			} else if (sessiontype == MM_SESSION_TYPE_RECORD_VIDEO) {
				ASM_unregister_sound(g_asm_handle, ASM_EVENT_MMCAMCORDER_VIDEO, &error);
			} else {
				LOCK(g_mutex_monitor);
				ASM_unregister_sound(g_monitor_asm_handle, ASM_EVENT_MONITOR, &error);
				UNLOCK(g_mutex_monitor);
			}
			g_asm_handle = -1;
			g_session_type = -1;
			return result;
		}
	}

	debug_fleave();

	return MM_ERROR_NONE;

REGISTER_FAILURE:
	debug_error("failed to ASM_register_sound(), sessiontype(%d), error(0x%x)", sessiontype, error);
	switch (error) {
	case ERR_ASM_POLICY_CANNOT_PLAY:
		return MM_ERROR_POLICY_BLOCKED;
	case ERR_ASM_POLICY_CANNOT_PLAY_BY_CALL:
		return MM_ERROR_POLICY_BLOCKED_BY_CALL;
	case ERR_ASM_POLICY_CANNOT_PLAY_BY_ALARM:
		return MM_ERROR_POLICY_BLOCKED_BY_ALARM;
	default:
		break;
	}
	return MM_ERROR_INVALID_HANDLE;
}

EXPORT_API
int mm_session_update_option(session_update_type_t update_type, int options)
{
	int error = 0;
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
	int result = MM_ERROR_NONE;
	int error = 0;
	int sessiontype = 0;

	debug_fenter();

	result = _mm_session_util_read_type(-1, &sessiontype);
	if (result) {
		debug_error("failed to _mm_session_util_read_type(), result(%d), maybe session is not created", result);
		return result;
	}
	debug_log("type : %d", sessiontype);

	if (sessiontype < MM_SESSION_TYPE_MEDIA || sessiontype >= MM_SESSION_TYPE_NUM) {
		debug_error("Invalid session type %d", sessiontype);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (watchevent < MM_SESSION_WATCH_EVENT_CALL || watchevent >= MM_SESSION_WATCH_EVENT_NUM) {
		debug_error("Invalid watch event %d", watchevent);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (watchstate < MM_SESSION_WATCH_STATE_STOP || watchstate >= MM_SESSION_WATCH_STATE_NUM) {
		debug_error("Invalid watch state %d", watchstate);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Register a watch callback */
	if (NULL == callback) {
		debug_error("Null callback function");
	} else {
		g_watch_data.fn = callback;
		g_watch_data.data = user_param;
		if (!ASM_set_watch_session (-1,	_translate_from_mm_session_to_asm_event(watchevent), _translate_from_mm_session_to_asm_state(watchstate), asm_watch_callback, (void*)&g_watch_data, &error)) {
			debug_error("Could not register a watcher(event:%d, state:%d), error(0x%x)", watchevent, watchstate, error);
			return MM_ERROR_INVALID_HANDLE;
		}
	}

	debug_fleave();

	return result;
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
	int error = 0;
	int result = MM_ERROR_NONE;
	int sessiontype = MM_SESSION_TYPE_MEDIA;
	ASM_sound_states_t state = ASM_STATE_NONE;

	debug_fenter();

	result = _mm_session_util_read_type(-1, &sessiontype);
	if (MM_ERROR_NONE != result) {
		debug_error("Can not read current type");
		DESTROY(g_mutex_monitor);
		return result;
	}

	/* Unregister call session type here */
	if (sessiontype == MM_SESSION_TYPE_CALL) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_CALL, &error)) {
			goto INVALID_HANDLE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_VIDEOCALL) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_VIDEOCALL, &error)) {
			goto INVALID_HANDLE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_VOIP) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_VOIP, &error)) {
			goto INVALID_HANDLE;
		}
	}
	/* Unregister advanced session type here */
	else if (sessiontype == MM_SESSION_TYPE_VOICE_RECOGNITION) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_VOICE_RECOGNITION, &error)) {
			goto INVALID_HANDLE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_RECORD_AUDIO) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_MMCAMCORDER_AUDIO, &error)) {
			goto INVALID_HANDLE;
		}
	} else if (sessiontype == MM_SESSION_TYPE_RECORD_VIDEO) {
		if (!ASM_unregister_sound(g_asm_handle, ASM_EVENT_MMCAMCORDER_VIDEO, &error)) {
			goto INVALID_HANDLE;
		}
	}
	if (g_asm_handle != -1) {
		g_asm_handle = -1;
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
				case ASM_STATE_NONE:
					break;
				case ASM_STATE_PLAYING:
				case ASM_STATE_WAITING:
				case ASM_STATE_STOP:
				case ASM_STATE_PAUSE:
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
				g_monitor_data.fn = NULL;
				g_monitor_data.data = NULL;
			}
		}
		UNLOCK(g_mutex_monitor);
		DESTROY(g_mutex_monitor);
	}

	result = _mm_session_util_delete_information(-1);
	if(result != MM_ERROR_NONE)
		return result;

	debug_fleave();

	return MM_ERROR_NONE;

INVALID_HANDLE:
	DESTROY(g_mutex_monitor);
	debug_error("failed to ASM_unregister_sound(), sessiontype(%d)", sessiontype);
	return MM_ERROR_INVALID_HANDLE;
}

EXPORT_API
int mm_session_remove_watch_callback(int watchevent, int watchstate)
{
	int result = MM_ERROR_NONE;
	int error = 0;
	int sessiontype = 0;

	debug_fenter();

	result = _mm_session_util_read_type(-1, &sessiontype);
	if(result) {
		debug_error("failed to _mm_session_util_read_type(), result(%d), maybe session is not created", result);
		return result;
	}
	debug_log("type : %d", sessiontype);

	if(sessiontype < MM_SESSION_TYPE_MEDIA || sessiontype >= MM_SESSION_TYPE_NUM) {
		debug_error("Invalid session type %d", sessiontype);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(watchevent < MM_SESSION_WATCH_EVENT_CALL || watchevent >= MM_SESSION_WATCH_EVENT_NUM) {
		debug_error("Invalid watch event %d", watchevent);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(watchstate < MM_SESSION_WATCH_STATE_STOP || watchstate >= MM_SESSION_WATCH_STATE_NUM) {
		debug_error("Invalid watch state %d", watchstate);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Unregister a watch callback */
	if(!ASM_unset_watch_session (_translate_from_mm_session_to_asm_event(watchevent), _translate_from_mm_session_to_asm_state(watchstate), &error)) {
		debug_error("Could not unregister a watcher(event:%d, state:%d), error(0x%x)", watchevent, watchstate, error);
		return MM_ERROR_INVALID_HANDLE;
	} else {
		g_watch_data.fn = NULL;
		g_watch_data.data = NULL;
	}

	debug_fleave();

	return result;
}

EXPORT_API
int mm_session_set_subsession(mm_subsession_t subsession, mm_subsession_option_t option)
{
	int error = 0;
	int ret = 0;

	debug_fenter();

	if (g_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}
	if (g_session_type == -1 || g_session_type < MM_SESSION_TYPE_CALL || g_session_type >= MM_SESSION_TYPE_NUM ) {
		debug_error ("session type is null, or out of bound(%d)\n", g_session_type);
		return MM_ERROR_INVALID_HANDLE;
	}

	if (subsession >= MM_SUBSESSION_TYPE_VOICE && subsession <= MM_SUBSESSION_TYPE_MEDIA) {
		if (g_session_type < MM_SESSION_TYPE_CALL || g_session_type > MM_SESSION_TYPE_VOIP) {
			debug_error ("Not support this subsession(%d) of CALL session_type(%d)\n", subsession, g_session_type);
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if (subsession == MM_SUBSESSION_TYPE_INIT) {
		if (g_session_type != MM_SESSION_TYPE_VOICE_RECOGNITION &&
			g_session_type != MM_SESSION_TYPE_RECORD_AUDIO &&
			g_session_type != MM_SESSION_TYPE_RECORD_VIDEO) {
			debug_error ("Not support this subsession(%d) of the session_type(%d)\n", subsession, g_session_type);
			return MM_ERROR_INVALID_HANDLE;
		}
	} else if (subsession == MM_SUBSESSION_TYPE_RECORD_STEREO ||
				subsession == MM_SUBSESSION_TYPE_RECORD_MONO) {
		if (g_session_type < MM_SESSION_TYPE_RECORD_AUDIO || g_session_type > MM_SESSION_TYPE_RECORD_VIDEO) {
			debug_error ("Not support this subsession(%d) of the session_type(%d)\n", subsession, g_session_type);
			return MM_ERROR_INVALID_HANDLE;
		}
	}

	if(option < MM_SUBSESSION_OPTION_NONE || option >= MM_SUBSESSION_OPTION_NUM) {
		debug_error ("option(%d) is not valid\n", option);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = ASM_set_subsession (g_asm_handle, (ASM_sound_sub_sessions_t)subsession, option, &error);
	if (!ret) {
		debug_error("ASM_set_subsession() failed, Set subsession to [%d] failed 0x%X\n\n", subsession, error);
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_get_subsession(mm_subsession_t *subsession)
{
	int error = 0;
	int ret = 0;

	debug_fenter();

	if(g_asm_handle == -1) {
		debug_error ("call session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}
	if(g_session_type == -1 || g_session_type < MM_SESSION_TYPE_CALL || g_session_type >= MM_SESSION_TYPE_NUM ) {
		debug_error ("Not support this session_type(%d)\n", g_session_type);
		return MM_ERROR_INVALID_HANDLE;
	}

	ret = ASM_get_subsession (g_asm_handle, (ASM_sound_sub_sessions_t*)subsession, &error);
	if (!ret) {
		debug_error("ASM_get_subsession() failed 0x%X\n\n", error);
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_log("ASM_get_subsession returned [%d]\n", *subsession);
	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_set_subevent(mm_session_sub_t subevent)
{
	int error = 0;
	int ret = 0;

	debug_fenter();

	if(g_asm_handle == -1) {
		debug_error ("session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}
	if(g_session_type == -1 ||  g_session_type < MM_SESSION_TYPE_VOICE_RECOGNITION || g_session_type >= MM_SESSION_TYPE_NUM ) {
		debug_error ("not support this session_type(%d)\n", g_session_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = ASM_set_subevent (g_asm_handle, (ASM_sound_sub_events_t)subevent, &error);
	if (!ret) {
		debug_error("ASM_set_subevent() failed, Set subevent to [%d] failed 0x%X\n\n", subevent, error);
		switch (error) {
		case ERR_ASM_POLICY_CANNOT_PLAY:
		case ERR_ASM_POLICY_CANNOT_PLAY_BY_PROFILE:
		case ERR_ASM_POLICY_CANNOT_PLAY_BY_CUSTOM:
			return MM_ERROR_POLICY_BLOCKED;
		case ERR_ASM_POLICY_CANNOT_PLAY_BY_CALL:
			return MM_ERROR_POLICY_BLOCKED_BY_CALL;
		case ERR_ASM_POLICY_CANNOT_PLAY_BY_ALARM:
			return MM_ERROR_POLICY_BLOCKED_BY_ALARM;
		}
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_get_subevent(mm_session_sub_t *subevent)
{
	int error = 0;
	int ret = 0;
	debug_fenter();

	if(g_asm_handle == -1) {
		debug_error ("session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}
	if(g_session_type == -1 ||  g_session_type < MM_SESSION_TYPE_VOICE_RECOGNITION || g_session_type >= MM_SESSION_TYPE_NUM ) {
		debug_error ("not support this session_type(%d)\n", g_session_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = ASM_get_subevent (g_asm_handle, (ASM_sound_sub_events_t*)subevent, &error);
	if (!ret) {
		debug_error("ASM_get_subevent() failed 0x%X\n\n", error);
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_log("ASM_get_subevent returned [%d]\n", *subevent);
	debug_fleave();

	return MM_ERROR_NONE;
}

EXPORT_API
int mm_session_reset_resumption_info(void)
{
	int error = 0;
	int ret = 0;

	debug_fenter();

	if(g_asm_handle == -1) {
		debug_error ("call series or voice recognition session is not started...\n");
		return MM_ERROR_INVALID_HANDLE;
	}
	if(g_session_type == -1 || g_session_type < MM_SESSION_TYPE_MEDIA || g_session_type >= MM_SESSION_TYPE_NUM ) {
		debug_error ("not support this session_type(%d)\n", g_session_type);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	ret = ASM_reset_resumption_info (g_asm_handle, &error);
	if (!ret) {
		debug_error("ASM_reset_resumption_info() failed 0x%X\n\n", error);
		return MM_ERROR_SOUND_INTERNAL;
	}

	debug_leave();

	return MM_ERROR_NONE;
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

gboolean _asm_monitor_cb(gpointer *data)
{
	session_monitor_t* monitor = (session_monitor_t*)data;
	debug_fenter();

	if (monitor) {
		if (monitor->fn) {
			monitor->fn(monitor->msg, monitor->event, monitor->data);
		}
	}
	debug_fleave();

	return FALSE;
}

gboolean _asm_watch_cb(gpointer *data)
{
	session_watch_t* watch_h = (session_watch_t*)data;
	debug_fenter();

	if (watch_h) {
		if (watch_h->fn) {
			watch_h->fn(watch_h->event, watch_h->state, watch_h->data);
		}
	}
	debug_fleave();

	return FALSE;
}

static session_event_t _translate_from_event_src_to_mm_session(ASM_event_sources_t event_src)
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

	case ASM_EVENT_SOURCE_NOTIFY_START:
	case ASM_EVENT_SOURCE_NOTIFY_END:
		return MM_SESSION_EVENT_NOTIFICATION;

	case ASM_EVENT_SOURCE_EMERGENCY_START:
	case ASM_EVENT_SOURCE_EMERGENCY_END:
		return MM_SESSION_EVENT_EMERGENCY;

	case ASM_EVENT_SOURCE_MEDIA:
	case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
	default:
		return MM_SESSION_EVENT_MEDIA;
	}
}

static session_watch_event_t _translate_from_asm_event_to_mm_session(ASM_sound_events_t sound_event)
{
	switch (sound_event)
	{
	case ASM_EVENT_CALL:
		return MM_SESSION_WATCH_EVENT_CALL;
	case ASM_EVENT_VIDEOCALL:
		return MM_SESSION_WATCH_EVENT_VIDEO_CALL;
	case ASM_EVENT_ALARM:
		return MM_SESSION_WATCH_EVENT_ALARM;
	default:
		return MM_SESSION_WATCH_EVENT_IGNORE;
	}
}

static ASM_sound_events_t _translate_from_mm_session_to_asm_event(session_watch_event_t watch_event)
{
	switch (watch_event)
	{
	case MM_SESSION_WATCH_EVENT_CALL:
		return ASM_EVENT_CALL;
	case MM_SESSION_WATCH_EVENT_VIDEO_CALL:
		return ASM_EVENT_VIDEOCALL;
	case MM_SESSION_WATCH_EVENT_ALARM:
		return ASM_EVENT_ALARM;
	default:
		return ASM_EVENT_NONE;
	}
}

static ASM_sound_states_t _translate_from_mm_session_to_asm_state(session_watch_state_t watch_state)
{
	switch (watch_state)
	{
	case MM_SESSION_WATCH_STATE_STOP:
		return ASM_STATE_STOP;
	case MM_SESSION_WATCH_STATE_PLAYING:
		return ASM_STATE_PLAYING;
	default:
		return ASM_STATE_NONE;
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
			monitor->event = _translate_from_event_src_to_mm_session (event_src);
			g_idle_add((GSourceFunc)_asm_monitor_cb, (gpointer)monitor);
		}
		cb_res = (command == ASM_COMMAND_STOP)? ASM_CB_RES_STOP : ASM_CB_RES_PAUSE;
		break;

	case ASM_COMMAND_RESUME:
	case ASM_COMMAND_PLAY:
		//call session_callback_fn for resume here
		if(monitor->fn) {
			monitor->msg = MM_SESSION_MSG_RESUME;
			monitor->event = _translate_from_event_src_to_mm_session (event_src);
			g_idle_add((GSourceFunc)_asm_monitor_cb, (gpointer)monitor);
		}
		cb_res = ASM_CB_RES_IGNORE;
		break;

	default:
		break;
	}
	return cb_res;
}

ASM_cb_result_t
asm_watch_callback(int handle, ASM_sound_events_t sound_event, ASM_sound_states_t sound_state, void* cb_data)
{
	ASM_cb_result_t cb_res = ASM_CB_RES_NONE;
	session_watch_t *watch_handle = (session_watch_t*)cb_data;

	debug_log("watch callback called for handle %d, sound_event %d, sound_state %d", handle, sound_event, sound_state);
	if(!watch_handle) {
		debug_log("watch_handle instance is null\n");
		return ASM_CB_RES_IGNORE;
	}

	switch(sound_state) {
	case ASM_STATE_PLAYING:
		if(watch_handle->fn) {
			watch_handle->event = _translate_from_asm_event_to_mm_session(sound_event);
			watch_handle->state = MM_SESSION_WATCH_STATE_PLAYING;
			if (watch_handle->event == MM_SESSION_WATCH_EVENT_IGNORE) {
				cb_res = ASM_CB_RES_IGNORE;
				debug_error("sound_event(%d) is not valid..", sound_event);
			} else {
				g_idle_add((GSourceFunc)_asm_watch_cb, (gpointer)watch_handle);
			}
		}
		break;
	case ASM_STATE_STOP:
		if(watch_handle->fn) {
			watch_handle->event = _translate_from_asm_event_to_mm_session(sound_event);
			watch_handle->state = MM_SESSION_WATCH_STATE_STOP;
			if (watch_handle->event == MM_SESSION_WATCH_EVENT_IGNORE) {
				debug_error("sound_event(%d) is not valid..", sound_event);
				cb_res = ASM_CB_RES_IGNORE;
			} else {
				g_idle_add((GSourceFunc)_asm_watch_cb, (gpointer)watch_handle);
			}
		}
		break;
	default:
		debug_error("sound_state(%d) is not valid..", sound_state);
		cb_res = ASM_CB_RES_IGNORE;
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
				g_monitor_data.fn = NULL;
				g_monitor_data.data = NULL;
			}
		}
		UNLOCK(g_mutex_monitor);
		DESTROY(g_mutex_monitor);
	}
	_mm_session_util_delete_information(-1);

	debug_fleave();
}

__attribute__ ((constructor))
void __mmsession_initialize(void)
{

}

