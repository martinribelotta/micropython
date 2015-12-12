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
#include "board.h"
#include "modpyb.h"
#include "ciaanxp_mphal.h"

typedef struct _pyb_eeprom_obj_t {
    mp_obj_base_t base;
} pyb_eeprom_obj_t;

STATIC pyb_eeprom_obj_t pyb_eeprom_obj[] = {
    {{&pyb_eeprom_type}}
};

#define EEPROM_ID(obj) ((obj) - &pyb_eeprom_obj[0])

void pyb_eeprom_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_eeprom_obj_t *self = self_in;
    mp_printf(print, "EEPROM (%u)", EEPROM_ID(self));
}

STATIC mp_obj_t pyb_eeprom_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return (mp_obj_t)&pyb_eeprom_obj[0];
}


// read_byte method
mp_obj_t pyb_eeprom_read_byte(mp_obj_t self_in,mp_obj_t mpAddr) {
    uint32_t addr = mp_obj_get_int(mpAddr);
    return MP_OBJ_NEW_SMALL_INT(mp_hal_readByteEEPROM(addr));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_eeprom_read_byte_obj, pyb_eeprom_read_byte);


// write_byte method
mp_obj_t pyb_eeprom_write_byte(mp_obj_t self_in,mp_obj_t mpAddr,mp_obj_t mpValue) {
    uint32_t addr = mp_obj_get_int(mpAddr);
    uint8_t value = (uint8_t)mp_obj_get_int(mpValue);
    mp_hal_writeByteEEPROM(addr,value);
    return MP_OBJ_NEW_SMALL_INT(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_eeprom_write_byte_obj, pyb_eeprom_write_byte);


// read_int method
mp_obj_t pyb_eeprom_read_int(mp_obj_t self_in,mp_obj_t mpAddr) {
    uint32_t addr = mp_obj_get_int(mpAddr);
    uint8_t valArray[4];
    valArray[0] = mp_hal_readByteEEPROM(addr);
    valArray[1] = mp_hal_readByteEEPROM(addr+1);
    valArray[2] = mp_hal_readByteEEPROM(addr+2);
    valArray[3] = mp_hal_readByteEEPROM(addr+3);

    return MP_OBJ_NEW_SMALL_INT(valArray[3]<<24 | valArray[2]<<16 | valArray[1]<<8 | valArray[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_eeprom_read_int_obj, pyb_eeprom_read_int);

// write_int method
mp_obj_t pyb_eeprom_write_int(mp_obj_t self_in,mp_obj_t mpAddr,mp_obj_t mpValue) {
    uint32_t addr = mp_obj_get_int(mpAddr);
    uint32_t value = (uint32_t)mp_obj_get_int(mpValue);
    uint8_t valArray[4];
    valArray[0] = value&0x000000FF;
    valArray[1] = (value>>8)&0x000000FF;
    valArray[2] = (value>>16)&0x000000FF;
    valArray[3] = (value>>24)&0x000000FF;

    mp_hal_writeByteEEPROM(addr,valArray[0]);
    mp_hal_writeByteEEPROM(addr+1,valArray[1]);
    mp_hal_writeByteEEPROM(addr+2,valArray[2]);
    mp_hal_writeByteEEPROM(addr+3,valArray[3]);

    return MP_OBJ_NEW_SMALL_INT(4);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_eeprom_write_int_obj, pyb_eeprom_write_int);


// read_float method
mp_obj_t pyb_eeprom_read_float(mp_obj_t self_in,mp_obj_t mpAddr) {
    uint32_t addr = mp_obj_get_int(mpAddr);
    float val;
    uint8_t* pFloat = (uint8_t*)&val;
    pFloat[0] = mp_hal_readByteEEPROM(addr);
    pFloat[1] = mp_hal_readByteEEPROM(addr+1);
    pFloat[2] = mp_hal_readByteEEPROM(addr+2);
    pFloat[3] = mp_hal_readByteEEPROM(addr+3);

    return mp_obj_new_float(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_eeprom_read_float_obj, pyb_eeprom_read_float);


// write_float method
mp_obj_t pyb_eeprom_write_float(mp_obj_t self_in,mp_obj_t mpAddr,mp_obj_t mpValue) {

    uint32_t addr = mp_obj_get_int(mpAddr);

    float value = (float)mp_obj_get_float(mpValue);

    uint8_t* pFloat = (uint8_t*)&value;

    mp_hal_writeByteEEPROM(addr,pFloat[0]);
    mp_hal_writeByteEEPROM(addr+1,pFloat[1]);
    mp_hal_writeByteEEPROM(addr+2,pFloat[2]);
    mp_hal_writeByteEEPROM(addr+3,pFloat[3]);

    return MP_OBJ_NEW_SMALL_INT(4);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_eeprom_write_float_obj, pyb_eeprom_write_float);



STATIC const mp_map_elem_t pyb_eeprom_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_byte), (mp_obj_t)&pyb_eeprom_read_byte_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_int), (mp_obj_t)&pyb_eeprom_read_int_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_float), (mp_obj_t)&pyb_eeprom_read_float_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_byte), (mp_obj_t)&pyb_eeprom_write_byte_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_int), (mp_obj_t)&pyb_eeprom_write_int_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_float), (mp_obj_t)&pyb_eeprom_write_float_obj },
};

STATIC MP_DEFINE_CONST_DICT(pyb_eeprom_locals_dict, pyb_eeprom_locals_dict_table);

const mp_obj_type_t pyb_eeprom_type = {
    { &mp_type_type },
    .name = MP_QSTR_EEPROM,
    .print = pyb_eeprom_print,
    .make_new = pyb_eeprom_make_new,
    .locals_dict = (mp_obj_t)&pyb_eeprom_locals_dict,
};
