#include "lua-pdfio.h"

static int l_array_len(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_pushinteger(L, (lua_Integer)pdfioArrayGetSize(larray->array));
	return 1;
}

static int l_array_get(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);

	if (lua_isinteger(L, 2)) {
		lua_Integer n = lua_tointeger(L, 2);
		if (n < 1 || (size_t)n > pdfioArrayGetSize(larray->array)) {
			lua_pushnil(L);
			return 1;
		}
		size_t idx = (size_t)n - 1;
		pdfio_valtype_t type = pdfioArrayGetType(larray->array, idx);
		switch (type) {
			case PDFIO_VALTYPE_ARRAY: lua_pdfio_push_value(L, type, pdfioArrayGetArray(larray->array, idx)); break;
			case PDFIO_VALTYPE_BINARY: {
				size_t length;
				unsigned char *data = pdfioArrayGetBinary(larray->array, idx, &length);
				lua_pushlstring(L, (const char *)data, length);
				break;
			}
			case PDFIO_VALTYPE_BOOLEAN: lua_pushboolean(L, pdfioArrayGetBoolean(larray->array, idx)); break;
			case PDFIO_VALTYPE_DATE: lua_pushinteger(L, (lua_Integer)pdfioArrayGetDate(larray->array, idx)); break;
			case PDFIO_VALTYPE_DICT: lua_pdfio_push_value(L, type, pdfioArrayGetDict(larray->array, idx)); break;
			case PDFIO_VALTYPE_INDIRECT: lua_pdfio_push_value(L, type, pdfioArrayGetObj(larray->array, idx)); break;
			case PDFIO_VALTYPE_NAME: lua_pushstring(L, pdfioArrayGetName(larray->array, idx)); break;
			case PDFIO_VALTYPE_NULL: lua_pushnil(L); break;
			case PDFIO_VALTYPE_NUMBER: lua_pushnumber(L, pdfioArrayGetNumber(larray->array, idx)); break;
			case PDFIO_VALTYPE_STRING: lua_pushstring(L, pdfioArrayGetString(larray->array, idx)); break;
			default: lua_pushnil(L); break;
		}
		return 1;
	}

	// Try methods
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	return 1;
}

static int l_array_append_boolean(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	bool val = lua_toboolean(L, 2);
	lua_pushboolean(L, pdfioArrayAppendBoolean(larray->array, val));
	return 1;
}

static int l_array_append_date(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	time_t val = (time_t)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioArrayAppendDate(larray->array, val));
	return 1;
}

static int l_array_append_name(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	const char *val = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioArrayAppendName(larray->array, val));
	return 1;
}

static int l_array_append_number(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	double val = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioArrayAppendNumber(larray->array, val));
	return 1;
}

static int l_array_append_string(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	const char *val = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioArrayAppendString(larray->array, val));
	return 1;
}

static int l_array_append_binary(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	size_t len;
	const unsigned char *val = (const unsigned char *)luaL_checklstring(L, 2, &len);
	lua_pushboolean(L, pdfioArrayAppendBinary(larray->array, val, len));
	return 1;
}

static int l_array_append_array(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_pdfio_array_t *lval = (lua_pdfio_array_t *)luaL_checkudata(L, 2, LUA_PDFIO_ARRAY_MT);
	lua_pushboolean(L, pdfioArrayAppendArray(larray->array, lval->array));
	return 1;
}

static int l_array_append_dict(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_pdfio_dict_t *lval = (lua_pdfio_dict_t *)luaL_checkudata(L, 2, LUA_PDFIO_DICT_MT);
	lua_pushboolean(L, pdfioArrayAppendDict(larray->array, lval->dict));
	return 1;
}

static int l_array_append_obj(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_pdfio_obj_t *lval = (lua_pdfio_obj_t *)luaL_checkudata(L, 2, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioArrayAppendObj(larray->array, lval->obj));
	return 1;
}

static int l_array_remove(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_Integer n = luaL_checkinteger(L, 2);
	if (n < 1 || (size_t)n > pdfioArrayGetSize(larray->array)) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, pdfioArrayRemove(larray->array, (size_t)n - 1));
	return 1;
}

static int l_array_copy(lua_State *L) {
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 1, LUA_PDFIO_ARRAY_MT);
	lua_pdfio_file_t *lfile = (lua_pdfio_file_t *)luaL_checkudata(L, 2, LUA_PDFIO_FILE_MT);
	pdfio_array_t *acopy = pdfioArrayCopy(lfile->pdf, larray->array);
	if (!acopy) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_array_t *lcopy = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	lcopy->array = acopy;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static const struct luaL_Reg lua_pdfio_array_methods[] = {
	{"append_boolean", l_array_append_boolean},
	{"append_date", l_array_append_date},
	{"append_name", l_array_append_name},
	{"append_number", l_array_append_number},
	{"append_string", l_array_append_string},
	{"append_binary", l_array_append_binary},
	{"append_array", l_array_append_array},
	{"append_dict", l_array_append_dict},
	{"append_obj", l_array_append_obj},
	{"remove", l_array_remove},
	{"copy", l_array_copy},
	{"__index", l_array_get},
	{"__len", l_array_len},
	{NULL, NULL}
};

void lua_pdfio_register_array(lua_State *L) {
	luaL_newmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_pdfio_array_methods, 0);
	lua_pop(L, 1);
}
