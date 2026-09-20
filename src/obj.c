#include "lua-pdfio.h"

static int l_obj_get_type(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushstring(L, pdfioObjGetType(lobj->obj));
	return 1;
}

static int l_obj_get_dict(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	pdfio_dict_t *dict = pdfioObjGetDict(lobj->obj);
	if (!dict) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_dict_t), 0);
	ldict->dict = dict;
	luaL_getmetatable(L, LUA_PDFIO_DICT_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_obj_get_num_streams(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushinteger(L, (lua_Integer)pdfioPageGetNumStreams(lobj->obj));
	return 1;
}

static int l_obj_open_page_stream(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	size_t n = (size_t)luaL_checkinteger(L, 2);
	bool decode = lua_toboolean(L, 3);
	pdfio_stream_t *stream = pdfioPageOpenStream(lobj->obj, n, decode);
	if (!stream) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_stream_t), 0);
	lstream->stream = stream;
	luaL_getmetatable(L, LUA_PDFIO_STREAM_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_obj_open_stream(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	bool decode = lua_toboolean(L, 2);
	pdfio_stream_t *stream = pdfioObjOpenStream(lobj->obj, decode);
	if (!stream) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_stream_t), 0);
	lstream->stream = stream;
	luaL_getmetatable(L, LUA_PDFIO_STREAM_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_obj_create_stream(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	int filter = (int)luaL_optinteger(L, 2, PDFIO_FILTER_NONE);
	pdfio_stream_t *stream = pdfioObjCreateStream(lobj->obj, (pdfio_filter_t)filter);
	if (!stream) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_stream_t), 0);
	lstream->stream = stream;
	luaL_getmetatable(L, LUA_PDFIO_STREAM_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_obj_image_get_bytes_per_line(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushinteger(L, (lua_Integer)pdfioImageGetBytesPerLine(lobj->obj));
	return 1;
}

static int l_obj_get_rect(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	const char *key = luaL_checkstring(L, 2);
	pdfio_rect_t rect;
	if (pdfioDictGetRect(pdfioObjGetDict(lobj->obj), key, &rect)) {
		lua_newtable(L);
		lua_pushnumber(L, rect.x1);
		lua_setfield(L, -2, "x1");
		lua_pushnumber(L, rect.y1);
		lua_setfield(L, -2, "y1");
		lua_pushnumber(L, rect.x2);
		lua_setfield(L, -2, "x2");
		lua_pushnumber(L, rect.y2);
		lua_setfield(L, -2, "y2");
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static int l_obj_image_get_width(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushnumber(L, pdfioImageGetWidth(lobj->obj));
	return 1;
}

static int l_obj_image_get_height(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushnumber(L, pdfioImageGetHeight(lobj->obj));
	return 1;
}

static int l_obj_text_measure(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	const char *s = luaL_checkstring(L, 2);
	double size = luaL_checknumber(L, 3);
	lua_pushnumber(L, pdfioContentTextMeasure(lobj->obj, s, size));
	return 1;
}

static int l_obj_close(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioObjClose(lobj->obj));
	return 1;
}

static int l_obj_get_generation(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushinteger(L, (lua_Integer)pdfioObjGetGeneration(lobj->obj));
	return 1;
}

static int l_obj_get_length(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushinteger(L, (lua_Integer)pdfioObjGetLength(lobj->obj));
	return 1;
}

static int l_obj_get_name(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushstring(L, pdfioObjGetName(lobj->obj));
	return 1;
}

static int l_obj_get_number(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushinteger(L, (lua_Integer)pdfioObjGetNumber(lobj->obj));
	return 1;
}

static int l_obj_get_subtype(lua_State *L) {
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 1, LUA_PDFIO_OBJ_MT);
	lua_pushstring(L, pdfioObjGetSubtype(lobj->obj));
	return 1;
}

static const struct luaL_Reg lua_pdfio_obj_methods[] = {
	{"get_type", l_obj_get_type},
	{"get_dict", l_obj_get_dict},
	{"get_num_streams", l_obj_get_num_streams},
	{"open_page_stream", l_obj_open_page_stream},
	{"open_stream", l_obj_open_stream},
	{"create_stream", l_obj_create_stream},
	{"get_rect", l_obj_get_rect},
	{"image_get_width", l_obj_image_get_width},
	{"image_get_height", l_obj_image_get_height},
	{"image_get_bytes_per_line", l_obj_image_get_bytes_per_line},
	{"text_measure", l_obj_text_measure},
	{"close", l_obj_close},
	{"get_generation", l_obj_get_generation},
	{"get_length", l_obj_get_length},
	{"get_name", l_obj_get_name},
	{"get_number", l_obj_get_number},
	{"get_subtype", l_obj_get_subtype},
	{NULL, NULL}
};

void lua_pdfio_register_obj(lua_State *L) {
	luaL_newmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_pdfio_obj_methods, 0);
	lua_pop(L, 1);
}
