#ifndef LUA_PDFIO_H
#define LUA_PDFIO_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <pdfio.h>
#include <pdfio-content.h>

/* Lua 5.1/5.2/5.3/5.4 Compatibility */
#if LUA_VERSION_NUM < 502
#define lua_rawlen lua_objlen
#define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#define luaL_setfuncs(L,l,n) (luaL_register(L,NULL,l))
#define lua_tointegerx(L,i,p) ((void)p, lua_tointeger(L,i))
static inline void *luaL_testudata(lua_State *L, int ud, const char *tname) {
	void *p = lua_touserdata(L, ud);
	if (p != NULL) {
		if (lua_getmetatable(L, ud)) {
			luaL_getmetatable(L, tname);
			if (!lua_rawequal(L, -1, -2)) p = NULL;
			lua_pop(L, 2);
			return p;
		}
	}
	return NULL;
}
#endif

#if LUA_VERSION_NUM < 503
#define lua_isinteger(L,n) lua_isnumber(L,n)
#endif

#if LUA_VERSION_NUM < 504
#define lua_newuserdatauv(L,s,u) ((void)u, lua_newuserdata(L,s))
#endif

#define LUA_PDFIO_FILE_MT   "lua_pdfio_file"
#define LUA_PDFIO_OBJ_MT    "lua_pdfio_obj"
#define LUA_PDFIO_DICT_MT   "lua_pdfio_dict"
#define LUA_PDFIO_ARRAY_MT  "lua_pdfio_array"
#define LUA_PDFIO_STREAM_MT "lua_pdfio_stream"

typedef struct {
	pdfio_file_t *pdf;
	void *output_ctx;
} lua_pdfio_file_t;

typedef struct {
	pdfio_obj_t *obj;
} lua_pdfio_obj_t;

typedef struct {
	pdfio_dict_t *dict;
} lua_pdfio_dict_t;

typedef struct {
	pdfio_array_t *array;
} lua_pdfio_array_t;

typedef struct {
	pdfio_stream_t *stream;
} lua_pdfio_stream_t;

// Shared helpers
void lua_pdfio_push_value(lua_State *L, pdfio_valtype_t type, void *value);

// Module functions
int l_file_open(lua_State *L);
int l_file_create(lua_State *L);
int l_file_create_output(lua_State *L);
int l_file_create_temporary(lua_State *L);
int l_file_array_create(lua_State *L);
int l_file_find_obj(lua_State *L);
int l_file_get_permissions(lua_State *L);
int l_file_set_permissions(lua_State *L);
int l_file_create_icc_obj_from_file(lua_State *L);
int l_file_create_icc_obj_from_data(lua_State *L);
int l_file_create_color_from_icc_obj(lua_State *L);
int l_file_create_color_from_standard(lua_State *L);
int l_file_create_color_from_palette(lua_State *L);
int l_file_create_color_from_matrix(lua_State *L);
int l_file_create_color_from_primaries(lua_State *L);

// Registration functions
void lua_pdfio_register_file(lua_State *L);
void lua_pdfio_register_obj(lua_State *L);
void lua_pdfio_register_dict(lua_State *L);
void lua_pdfio_register_array(lua_State *L);
void lua_pdfio_register_stream(lua_State *L);

#endif
