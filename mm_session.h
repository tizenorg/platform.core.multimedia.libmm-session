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
 * @file		mm_session.h
 * @author
 * @version		1.0
 * @brief		This file declares multimedia framework session type.
 */
#ifndef	_MM_SESSION_H_
#define	_MM_SESSION_H_

#include <audio-session-manager-types.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
	@addtogroup MMSESSION
	@{
	@par
	This part is describes multimedia framework session type and function
 */

/**
  * This enumeration defines application's session types.
  */
enum MMSessionType {
	MM_SESSION_TYPE_MEDIA = 0,
	MM_SESSION_TYPE_MEDIA_RECORD,
	MM_SESSION_TYPE_ALARM,
	MM_SESSION_TYPE_NOTIFY,
	MM_SESSION_TYPE_EMERGENCY,
	MM_SESSION_TYPE_CALL,
	MM_SESSION_TYPE_VIDEOCALL,
	MM_SESSION_TYPE_VOIP,
	MM_SESSION_TYPE_VOICE_RECOGNITION,
	MM_SESSION_TYPE_RECORD_AUDIO,
	MM_SESSION_TYPE_RECORD_VIDEO,
	MM_SESSION_TYPE_NUM
};

/**
  * This enumeration defines behavior of update.
  */
typedef enum {
	MM_SESSION_UPDATE_TYPE_ADD,
	MM_SESSION_UPDATE_TYPE_REMOVE,
	MM_SESSION_UPDATE_TYPE_NUM
}session_update_type_t;

/**
  * This define is for session options
  */
#define MM_SESSION_OPTION_PAUSE_OTHERS                      ASM_SESSION_OPTION_PAUSE_OTHERS
#define MM_SESSION_OPTION_UNINTERRUPTIBLE                   ASM_SESSION_OPTION_UNINTERRUPTIBLE
#define MM_SESSION_OPTION_RESUME_BY_SYSTEM_OR_MEDIA_PAUSED  ASM_SESSION_OPTION_RESUME_BY_MEDIA_PAUSED

/**
  * This enumeration defines session callback message type.
  */
typedef enum {
	MM_SESSION_MSG_STOP,	/**< Message Stop : this messages means that session of application has interrupted by policy.
												So application should stop it's multi-media context when this message has come */
	MM_SESSION_MSG_RESUME,	/**< Message Stop : this messages means that session interrupt of application has ended.
												So application could resume it's multi-media context when this message has come */
	MM_SESSION_MSG_NUM
}session_msg_t;

typedef enum {
	MM_SESSION_EVENT_MEDIA = 0,
	MM_SESSION_EVENT_CALL,
	MM_SESSION_EVENT_ALARM,
	MM_SESSION_EVENT_EARJACK_UNPLUG,
	MM_SESSION_EVENT_RESOURCE_CONFLICT,
	MM_SESSION_EVENT_EMERGENCY,
	MM_SESSION_EVENT_NOTIFICATION,
}session_event_t;

typedef enum {
	MM_SESSION_WATCH_EVENT_IGNORE = -1,
	MM_SESSION_WATCH_EVENT_CALL = 0,
	MM_SESSION_WATCH_EVENT_VIDEO_CALL,
	MM_SESSION_WATCH_EVENT_ALARM,
	MM_SESSION_WATCH_EVENT_NUM
}session_watch_event_t;

typedef enum {
	MM_SESSION_WATCH_STATE_STOP = 0,
	MM_SESSION_WATCH_STATE_PLAYING,
	MM_SESSION_WATCH_STATE_NUM
}session_watch_state_t;

typedef void (*session_callback_fn) (session_msg_t msg, session_event_t event, void *user_param);
typedef void (*watch_callback_fn) (session_watch_event_t event, session_watch_state_t state, void *user_param);

/**
 * This function defines application's Multimedia Session policy
 *
 * @param	sessiontype	[in] Multimedia Session type
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Session type is unique for each application (each PID actually).
 * 			if application want to change session type, Finish session first and Init again
 * @see		MMSessionType mm_session_finish
 * @since
 * @pre		There should be pre-initialized session type for caller application.
 * @post	A session type of caller application will be defined process widely.
 * @par Example
 * @code
#include <mm_session.h>

static int _create(void *data)
{
	int ret = 0;

	// Initialize Multimedia Session Type
	ret = mm_session_init(MM_SESSION_TYPE_MEDIA);
	if(ret < 0)
	{
		printf("Can not initialize session \n");
	}
	...
}

static int _terminate(void* data)
{
	int ret = 0;

	// Deinitialize Multimedia Session Type
	ret = mm_session_finish();
	if(ret < 0)
	{
		printf("Can not finish session\n");
	}
	...
}

int main()
{
	...
	struct appcore_ops ops = {
		.create = _create,
		.terminate = _terminate,
		.pause = _pause,
		.resume = _resume,
		.reset = _reset,
	};
	...
	return appcore_efl_main(PACKAGE, ..., &ops);
}

 * @endcode
 */
int mm_session_init(int sessiontype);




/**
 * This function defines application's Multimedia Session policy
 *
 * @param	sessiontype	[in] Multimedia Session type
 * @param	session_callback_fn [in] session message callback function pointer
 * @param	user_param [in] callback function user parameter
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Session type is unique for each application (each PID actually).
 * 			if application want to change session type, Finish session first and Init again
 * @pre		There should be pre-initialized session type for caller application.
 * @post	A session type of caller application will be defined process widely.
 * 			And session callback will be registered as given function pointer with given user_param
 * @see		MMSessionType mm_session_finish
 * @since
 * @par Example
 * @code
#include <mm_session.h>

session_callback_fn session_cb(session_msg_t msg,  session_event_t event, void *user_param)
{
	struct appdata* ad = (struct appdata*) user_param;

	switch(msg)
	{
	case MM_SESSION_MSG_STOP:
		// Stop multi-media context here
		...
		break;
	case MM_SESSION_MSG_RESUME:
		// Resume multi-media context here
		...
		break;
	default:
		break;
	}
	...
}

static int _create(void *data)
{
	struct appdata* ad = (struct appdata*) data;
	int ret = 0;

	// Initialize Multimedia Session Type with callback
	ret = mm_session_init_ex(MM_SESSION_TYPE_MEDIA, session_cb, (void*)ad);
	if(ret < 0)
	{
		printf("Can not initialize session \n");
	}
	...
}

static int _terminate(void* data)
{
	int ret = 0;

	// Deinitialize Multimedia Session Type
	ret = mm_session_finish();
	if(ret < 0)
	{
		printf("Can not finish session\n");
	}
	...
}


int main()
{
	...
	struct appcore_ops ops = {
		.create = _create,
		.terminate = _terminate,
		.pause = _pause,
		.resume = _resume,
		.reset = _reset,
	};
	...
	return appcore_efl_main(PACKAGE, ..., &ops);
}

 * @endcode
 */
int mm_session_init_ex(int sessiontype, session_callback_fn callback, void* user_param);



/**
 * This function finish application's Multimedia Session.
 *
 * 
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @remark	Session type is unique for each application (each PID actually).
 * 			if application want to change session type, Finish session first and Init again
 * @see		mm_session_init
 * @pre		A session type should be initialized for caller application.
 * @post	A session type for caller application will be cleared.
 * @since
 * @par Example
 * @code
#include <mm_session.h>

static int _create(void *data)
{
	int ret = 0;

	// Initialize Multimedia Session Type
	ret = mm_session_init(MM_SESSION_TYPE_MEDIA);
	if(ret < 0)
	{
		printf("Can not initialize session \n");
	}
	...
}

static int _terminate(void* data)
{
	int ret = 0;

	// Deinitialize Multimedia Session Type
	ret = mm_session_finish();
	if(ret < 0)
	{
		printf("Can not finish session\n");
	}
	...
}

int main()
{
	...
	struct appcore_ops ops = {
		.create = _create,
		.terminate = _terminate,
		.pause = _pause,
		.resume = _resume,
		.reset = _reset,
	};
	...
	return appcore_efl_main(PACKAGE, ..., &ops);
}
 * @endcode
 */
int mm_session_finish(void);

/**
 * This function get current application's Multimedia Session type
 *
 * @param	sessiontype	[out] Current Multimedia Session type
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @see		mm_session_init
 * @since
 */
int mm_session_get_current_type(int *sessiontype);

/**
 * This function get current application's Multimedia Session information
 *
 * @param	session_type	[out] Current Multimedia Session type
 * @param	session_options	[out] Current Multimedia Session options
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @see
 * @since
 */
int mm_session_get_current_information(int *session_type, int *session_options);

/**
 * This function update application's Multimedia Session options
 *
 * @param	update_type	[in] add or remove options
 * @param	session_options	[in] Multimedia Session options to be updated
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @see
 * @since
 */
int mm_session_update_option(session_update_type_t update_type, int options);

/**
 * This function add a watch callback
 *
 * @param	watchevent	[in]	The session type to be watched
 * @param	watchstate	[in]	The session state of the session type of first argument to be watched
 * @param	callback	[in]	The callback which will be called when the watched session state was activated
 * @param	user_param	[in]	The user param passed from the callback registration function
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @see
 * @since
 */
int mm_session_add_watch_callback(int watchevent, int watchstate, watch_callback_fn callback, void* user_param);

/**
 * This function removes a watch callback corresponding with two arguments
 *
 * @param	watchevent	[in]	The session type to be removed
 * @param	watchstate	[in]	The session state to be removed
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 * @see
 * @since
 */
int mm_session_remove_watch_callback(int watchevent, int watchstate);

/**
 * This function initialize resumption of other ASM handles which were paused by this session
 * It can be used only when call series or voice recognition session is set
 *
 * @return	This function returns MM_ERROR_NONE on success, or negative value
 *			with error code.
 */
int mm_session_reset_resumption_info(void);

/**
	@}
 */

#ifdef __cplusplus
}
#endif

#endif
