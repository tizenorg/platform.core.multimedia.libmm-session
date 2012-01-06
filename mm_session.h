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
 * @file		mm_session.h
 * @author
 * @version		1.0
 * @brief		This file declares multimedia framework session type.
 */
#ifndef	_MM_SESSION_H_
#define	_MM_SESSION_H_

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
	MM_SESSION_TYPE_SHARE = 0, 	/**< Share type : this type shares it's session with other share type application */
	MM_SESSION_TYPE_EXCLUSIVE,	/**< Exclusive type : this type make previous session stop. And it does not allow other share type session start */
	MM_SESSION_TYPE_NUM,
};

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

typedef void (*session_callback_fn) (session_msg_t msg, void *user_param);

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
	ret = mm_session_init(MM_SESSION_TYPE_SHARE);
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

session_callback_fn session_cb(session_msg_t msg, void *user_param)
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
	ret = mm_session_init_ex(MM_SESSION_TYPE_SHARE, session_cb, (void*)ad);
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
	ret = mm_session_init(MM_SESSION_TYPE_SHARE);
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
	@}
 */

#ifdef __cplusplus
}
#endif

#endif
