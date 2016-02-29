/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/runtime.h"
#include "py/stream.h"
#include "py/gc.h"
#include "board.h"
#include "modpyb.h"
#include "lpc43xx_mphal.h"


typedef struct _pyb_rtc_obj_t {
    mp_obj_base_t base;
    mp_obj_t callback;
} pyb_rtc_obj_t;

STATIC pyb_rtc_obj_t pyb_rtc_obj[] = {
    {{&pyb_rtc_type}}
};

#define RTC_ID(obj) ((obj) - &pyb_rtc_obj[0]) + 1

void pyb_rtc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_rtc_obj_t *self = self_in;
    mp_printf(print, "RTC (%u)", RTC_ID(self) );
}

STATIC void(*rtcCallback)(void*) = NULL;
STATIC void* rtcCallbackArg = NULL;
STATIC bool rtc_init = false;

void PYB_rtc_init(void) {
	if (rtc_init)
		return;
	Chip_RTC_Init(LPC_RTC);
	rtcCallback=NULL;
	Chip_RTC_CntIncrIntConfig(LPC_RTC,0, DISABLE);
	NVIC_EnableIRQ(RTC_IRQn);
	rtc_init = true;
}

STATIC mp_obj_t pyb_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    PYB_rtc_init();
    return (mp_obj_t)&pyb_rtc_obj[0];
}

/// \method datetime([datetimetuple])
STATIC mp_obj_t rtc_datetime(mp_uint_t n_args, const mp_obj_t *args) {
    //pyb_rtc_obj_t *self = args[0];

    if (n_args == 1) {
		mp_obj_t tuple[7];
		RTC_TIME_T rtc;
		Chip_RTC_GetFullTime(LPC_RTC, &rtc);

		tuple[0] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_SECOND]);
		tuple[1] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_MINUTE]);
		tuple[2] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_HOUR]);
		tuple[3] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_DAYOFMONTH]);
		tuple[4] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_MONTH]);
		tuple[5] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_YEAR]);
		tuple[6] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_DAYOFWEEK]);

		return mp_obj_new_tuple(7,tuple);
		
    } else  {
        // set 
        //(year, month, day, weekday, hours, minutes, seconds)
		mp_buffer_info_t bufinfo;
		mp_obj_t *items;
		mp_obj_get_array(args[1], &bufinfo.len, &items);
		
		if(bufinfo.len==7)
		{
			RTC_TIME_T rtc;
			rtc.time[RTC_TIMETYPE_SECOND]     = mp_obj_get_int(items[0]);
			rtc.time[RTC_TIMETYPE_MINUTE]     = mp_obj_get_int(items[1]);
			rtc.time[RTC_TIMETYPE_HOUR]       = mp_obj_get_int(items[2]);
			rtc.time[RTC_TIMETYPE_DAYOFMONTH] = mp_obj_get_int(items[3]);
			rtc.time[RTC_TIMETYPE_MONTH]      = mp_obj_get_int(items[4]);
			rtc.time[RTC_TIMETYPE_YEAR]       = mp_obj_get_int(items[5]);
			rtc.time[RTC_TIMETYPE_DAYOFWEEK]  = mp_obj_get_int(items[6]);
			Chip_RTC_SetFullTime(LPC_RTC, &rtc);
		}
		else{
			nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "7 arguments are required in tuple"));
		}
		
        return mp_const_none;	
    }
    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtc_datetime_obj, 1, 2, rtc_datetime);


/// \method alarm_datetime([datetimetuple,alarmMask])
STATIC mp_obj_t rtc_alarm_datetime(mp_uint_t n_args, const mp_obj_t *args) {
    //pyb_rtc_obj_t *self = args[0];

    if (n_args == 1) {

		mp_obj_t tuple[7];
		RTC_TIME_T rtc;
		Chip_RTC_GetFullAlarmTime(LPC_RTC, &rtc);

		tuple[0] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_SECOND]);
		tuple[1] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_MINUTE]);
		tuple[2] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_HOUR]);
		tuple[3] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_DAYOFMONTH]);
		tuple[4] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_MONTH]);
		tuple[5] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_YEAR]);
		tuple[6] = MP_OBJ_NEW_SMALL_INT(rtc.time[RTC_TIMETYPE_DAYOFWEEK]);

		return mp_obj_new_tuple(7,tuple);
		
    } else if(n_args == 3) {
        // set 
        //(year, month, day, weekday, hours, minutes, seconds)
		mp_buffer_info_t bufinfo;
		mp_obj_t *items;
		mp_obj_get_array(args[1], &bufinfo.len, &items);
		uint32_t alarmMask = mp_obj_get_int(args[2]);
		
		if(bufinfo.len==7)
		{
			RTC_TIME_T rtc;
			rtc.time[RTC_TIMETYPE_SECOND]     = mp_obj_get_int(items[0]);
			rtc.time[RTC_TIMETYPE_MINUTE]     = mp_obj_get_int(items[1]);
			rtc.time[RTC_TIMETYPE_HOUR]       = mp_obj_get_int(items[2]);
			rtc.time[RTC_TIMETYPE_DAYOFMONTH] = mp_obj_get_int(items[3]);
			rtc.time[RTC_TIMETYPE_MONTH]      = mp_obj_get_int(items[4]);
			rtc.time[RTC_TIMETYPE_YEAR]       = mp_obj_get_int(items[5]);
			rtc.time[RTC_TIMETYPE_DAYOFWEEK]  = mp_obj_get_int(items[6]);
			Chip_RTC_SetFullAlarmTime(LPC_RTC, &rtc);
	        Chip_RTC_AlarmIntConfig(LPC_RTC,alarmMask, ENABLE);
		}
		else{
			nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "7 arguments are required in tuple"));
		}
		
        return mp_const_none;	
    }
    else {
		nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "2 arguments are required"));
	}
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtc_alarm_datetime_obj, 1, 3, rtc_alarm_datetime);

/// \method alarm_disable()
STATIC mp_obj_t rtc_alarm_disable(mp_obj_t self_in) {
	Chip_RTC_AlarmIntConfig(LPC_RTC,0xFF, DISABLE);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rtc_alarm_disable_obj, rtc_alarm_disable);

void pyb_rtc_exec_callback(pyb_rtc_obj_t* self)
{
	// execute callback if it's set
	if (self->callback != mp_const_none) {
		// When executing code within a handler we must lock the GC to prevent
		// any memory allocations.  We must also catch any exceptions.
		gc_lock();
		nlr_buf_t nlr;
		if (nlr_push(&nlr) == 0) {
			mp_call_function_1(self->callback, self);
			nlr_pop();
		} else {
			// Uncaught exception; disable the callback so it doesn't run again.
			self->callback = mp_const_none;
			printf("uncaught exception in RTC(%u) interrupt handler\n",RTC_ID(self));
			mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
		}
		gc_unlock();
	}
}

/// \method callback(fun)
/// Set the function to be called in timer interrupt.
/// `fun` is passed 1 argument, the timer object.
/// If `fun` is `None` then the callback will be disabled.
STATIC mp_obj_t pyb_rtc_callback(mp_obj_t self_in, mp_obj_t callback) {
    pyb_rtc_obj_t *self = self_in;

    if (callback == mp_const_none) {
        // stop interrupt
        self->callback = mp_const_none;
    	rtcCallback = NULL;
    	rtcCallbackArg = NULL;
    } else if (mp_obj_is_callable(callback)) {
        self->callback = callback;
    	rtcCallback = ( void(*)(void*)  )pyb_rtc_exec_callback;
    	rtcCallbackArg = self;
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "callback must be None or a callable object"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtc_callback_obj, pyb_rtc_callback);

/// \method read_bkp_reg(addr)
STATIC mp_obj_t rtc_read_bkp_reg(mp_obj_t self_in, mp_obj_t addrObj) {
   
	uint8_t addr = (uint8_t)mp_obj_get_int(addrObj);
	
	uint32_t val = Chip_REGFILE_Read(LPC_REGFILE, addr);
	
	return MP_OBJ_NEW_SMALL_INT(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtc_read_bkp_reg_obj, rtc_read_bkp_reg);

/// \method write_bkp_write(addr,value)
STATIC mp_obj_t rtc_write_bkp_reg(mp_obj_t self_in, mp_obj_t addrObj, mp_obj_t valObj) {
   
	uint8_t addr = (uint8_t)mp_obj_get_int(addrObj);
	uint32_t val = (uint32_t)mp_obj_get_int(valObj);
	
	Chip_REGFILE_Write(LPC_REGFILE, addr, val);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(rtc_write_bkp_reg_obj, rtc_write_bkp_reg);


/// \method calibration(value)
STATIC mp_obj_t rtc_calibration(mp_obj_t self_in, mp_obj_t valObj) {
   
	uint32_t value = (uint32_t)mp_obj_get_int(valObj);
	
	uint8_t calibDir = RTC_CALIB_DIR_FORWARD;
	if(value<0)
	{
		calibDir = RTC_CALIB_DIR_BACKWARD;
		value=value*(-1);
	}

	if(value>131072)
		value=131072;

	Chip_RTC_CalibConfig(LPC_RTC, value, calibDir);
	Chip_RTC_CalibCounterCmd(LPC_RTC, ENABLE);
	Chip_RTC_Enable(LPC_RTC, ENABLE);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtc_calibration_obj, rtc_calibration);


STATIC const mp_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_datetime), (mp_obj_t)&rtc_datetime_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_alarm_datetime), (mp_obj_t)&rtc_alarm_datetime_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_bkp_reg), (mp_obj_t)&rtc_read_bkp_reg_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_bkp_reg), (mp_obj_t)&rtc_write_bkp_reg_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_calibration), (mp_obj_t)&rtc_calibration_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_alarm_disable), (mp_obj_t)&rtc_alarm_disable_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_callback), (mp_obj_t)&rtc_callback_obj },
    
    // Class Constants
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_SEC),         MP_OBJ_NEW_SMALL_INT((1 << 0)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_MIN),         MP_OBJ_NEW_SMALL_INT((1 << 1)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_HR),         MP_OBJ_NEW_SMALL_INT((1 << 2)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_DAY),         MP_OBJ_NEW_SMALL_INT((1 << 3)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_DOW),         MP_OBJ_NEW_SMALL_INT((1 << 4)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_MON),         MP_OBJ_NEW_SMALL_INT((1 << 6)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK_YR),         MP_OBJ_NEW_SMALL_INT((1 << 7)) },
    
};

STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);



const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .print = pyb_rtc_print,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_t)&pyb_rtc_locals_dict,
};
