#include "lua-pdfio.h"

static int l_dict_get(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);

	luaL_getmetatable(L, LUA_PDFIO_DICT_MT);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1)) return 1;
	lua_pop(L, 2);

	pdfio_valtype_t type = pdfioDictGetType(ldict->dict, key);
	if (type == PDFIO_VALTYPE_NONE) {
		lua_pushnil(L);
		return 1;
	}
	switch (type) {
		case PDFIO_VALTYPE_ARRAY: lua_pdfio_push_value(L, type, pdfioDictGetArray(ldict->dict, key)); break;
		case PDFIO_VALTYPE_BINARY: {
			size_t length;
			unsigned char *data = pdfioDictGetBinary(ldict->dict, key, &length);
			lua_pushlstring(L, (const char *)data, length);
			break;
		}
		case PDFIO_VALTYPE_BOOLEAN: lua_pushboolean(L, pdfioDictGetBoolean(ldict->dict, key)); break;
		case PDFIO_VALTYPE_DATE: lua_pushinteger(L, (lua_Integer)pdfioDictGetDate(ldict->dict, key)); break;
		case PDFIO_VALTYPE_DICT: lua_pdfio_push_value(L, type, pdfioDictGetDict(ldict->dict, key)); break;
		case PDFIO_VALTYPE_INDIRECT: lua_pdfio_push_value(L, type, pdfioDictGetObj(ldict->dict, key)); break;
		case PDFIO_VALTYPE_NAME: lua_pushstring(L, pdfioDictGetName(ldict->dict, key)); break;
		case PDFIO_VALTYPE_NULL: lua_pushnil(L); break;
		case PDFIO_VALTYPE_NUMBER: lua_pushnumber(L, pdfioDictGetNumber(ldict->dict, key)); break;
		case PDFIO_VALTYPE_STRING: lua_pushstring(L, pdfioDictGetString(ldict->dict, key)); break;
		default: lua_pushnil(L); break;
	}
	return 1;
}

static int l_dict_set_binary(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	size_t len;
	const unsigned char *val = (const unsigned char *)luaL_checklstring(L, 3, &len);
	lua_pushboolean(L, pdfioDictSetBinary(ldict->dict, key, val, len));
	return 1;
}

static int l_dict_clear(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioDictClear(ldict->dict, key));
	return 1;
}

static int l_dict_copy(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	lua_pdfio_file_t *lfile = (lua_pdfio_file_t *)luaL_checkudata(L, 2, LUA_PDFIO_FILE_MT);
	pdfio_dict_t *dcopy = pdfioDictCopy(lfile->pdf, ldict->dict);
	if (!dcopy) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_dict_t *lcopy = (lua_pdfio_dict_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_dict_t), 0);
	lcopy->dict = dcopy;
	luaL_getmetatable(L, LUA_PDFIO_DICT_MT);
	lua_setmetatable(L, -2);
	return 1;
}

typedef struct {
	lua_State *L;
	int fn_ref;
} lua_pdfio_dict_iter_ctx_t;

static bool dict_iterate_cb(pdfio_dict_t *dict, const char *key, void *cb_data) {
	(void)dict;
	lua_pdfio_dict_iter_ctx_t *ctx = (lua_pdfio_dict_iter_ctx_t *)cb_data;
	lua_State *L = ctx->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->fn_ref);
	lua_pushstring(L, key);
	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		fprintf(stderr, "lua-pdfio: iterate_keys callback error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	bool cont = lua_toboolean(L, -1) || lua_isnil(L, -1);
	lua_pop(L, 1);
	return cont;
}

static int l_dict_iterate_keys(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	lua_pdfio_dict_iter_ctx_t ctx;
	ctx.L = L;
	lua_pushvalue(L, 2);
	ctx.fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pdfioDictIterateKeys(ldict->dict, dict_iterate_cb, &ctx);
	luaL_unref(L, LUA_REGISTRYINDEX, ctx.fn_ref);
	return 0;
}

static int l_dict_set_boolean(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	bool val = lua_toboolean(L, 3);
	lua_pushboolean(L, pdfioDictSetBoolean(ldict->dict, key, val));
	return 1;
}

static int l_dict_set_date(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	time_t val = (time_t)luaL_checkinteger(L, 3);
	lua_pushboolean(L, pdfioDictSetDate(ldict->dict, key, val));
	return 1;
}

static int l_dict_set_name(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	const char *val = luaL_checkstring(L, 3);
	lua_pushboolean(L, pdfioDictSetName(ldict->dict, key, val));
	return 1;
}

static int l_dict_set_number(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	double val = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioDictSetNumber(ldict->dict, key, val));
	return 1;
}

static int l_dict_set_string(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	const char *val = luaL_checkstring(L, 3);
	lua_pushboolean(L, pdfioDictSetString(ldict->dict, key, val));
	return 1;
}

static int l_dict_set_null(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioDictSetNull(ldict->dict, key));
	return 1;
}

static int l_dict_set_array(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 3, LUA_PDFIO_ARRAY_MT);
	lua_pushboolean(L, pdfioDictSetArray(ldict->dict, key, larray->array));
	return 1;
}

static int l_dict_set_dict(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	lua_pdfio_dict_t *lval = (lua_pdfio_dict_t *)luaL_checkudata(L, 3, LUA_PDFIO_DICT_MT);
	lua_pushboolean(L, pdfioDictSetDict(ldict->dict, key, lval->dict));
	return 1;
}

static int l_dict_set_obj(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 3, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioDictSetObj(ldict->dict, key, lobj->obj));
	return 1;
}

static int l_dict_set_rect(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *key = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	pdfio_rect_t rect;
	lua_getfield(L, 3, "x1"); rect.x1 = luaL_checknumber(L, -1); lua_pop(L, 1);
	lua_getfield(L, 3, "y1"); rect.y1 = luaL_checknumber(L, -1); lua_pop(L, 1);
	lua_getfield(L, 3, "x2"); rect.x2 = luaL_checknumber(L, -1); lua_pop(L, 1);
	lua_getfield(L, 3, "y2"); rect.y2 = luaL_checknumber(L, -1); lua_pop(L, 1);
	lua_pushboolean(L, pdfioDictSetRect(ldict->dict, key, &rect));
	return 1;
}

static int l_dict_keys(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	size_t num = pdfioDictGetNumPairs(ldict->dict);
	lua_newtable(L);
	for (size_t i = 0; i < num; i++) {
		lua_pushstring(L, pdfioDictGetKey(ldict->dict, i));
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

static int l_dict_add_font(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *name = luaL_checkstring(L, 2);
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 3, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioPageDictAddFont(ldict->dict, name, lobj->obj));
	return 1;
}

static int l_dict_add_image(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *name = luaL_checkstring(L, 2);
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 3, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioPageDictAddImage(ldict->dict, name, lobj->obj));
	return 1;
}

static int l_dict_add_color_space(lua_State *L) {
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 1, LUA_PDFIO_DICT_MT);
	const char *name = luaL_checkstring(L, 2);
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 3, LUA_PDFIO_ARRAY_MT);
	lua_pushboolean(L, pdfioPageDictAddColorSpace(ldict->dict, name, larray->array));
	return 1;
}

static const struct luaL_Reg lua_pdfio_dict_methods[] = {
	{"add_font", l_dict_add_font},
	{"add_image", l_dict_add_image},
	{"add_color_space", l_dict_add_color_space},
	{"set_boolean", l_dict_set_boolean},
	{"set_date", l_dict_set_date},
	{"set_name", l_dict_set_name},
	{"set_number", l_dict_set_number},
	{"set_string", l_dict_set_string},
	{"set_null", l_dict_set_null},
	{"set_array", l_dict_set_array},
	{"set_dict", l_dict_set_dict},
	{"set_obj", l_dict_set_obj},
	{"set_rect", l_dict_set_rect},
	{"set_binary", l_dict_set_binary},
	{"clear", l_dict_clear},
	{"copy", l_dict_copy},
	{"iterate_keys", l_dict_iterate_keys},
	{"keys", l_dict_keys},
	{"__index", l_dict_get},
	{NULL, NULL}
};

void lua_pdfio_register_dict(lua_State *L) {
	luaL_newmetatable(L, LUA_PDFIO_DICT_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_pdfio_dict_methods, 0);
	lua_pop(L, 1);
}
