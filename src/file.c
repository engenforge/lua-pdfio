#include "lua-pdfio.h"
#include <string.h>
#include <time.h>

static bool error_cb(pdfio_file_t *pdf, const char *message, void *data) {
	(void)pdf;
	(void)data;
	if (strstr(message, "Unable to unlock PDF file")) return true;
	fprintf(stderr, "pdfio error: %s\n", message);
	return true;
}

static const char *password_cb(void *data, const char *filename) {
	(void)filename;
	return (const char *)data;
}

int l_file_open(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *password = luaL_optstring(L, 2, NULL);
	pdfio_file_t *pdf = pdfioFileOpen(filename, password_cb, (void *)password, error_cb, NULL);
	if (!pdf) {
		lua_pushnil(L);
		lua_pushfstring(L, "Failed to open PDF file: %s", filename);
		return 2;
	}
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_file_t), 0);
	lpdf->pdf = pdf;
	lpdf->output_ctx = NULL;
	luaL_getmetatable(L, LUA_PDFIO_FILE_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *version = luaL_optstring(L, 2, "1.7");
	pdfio_file_t *pdf = pdfioFileCreate(filename, version, NULL, NULL, error_cb, NULL);
	if (!pdf) {
		lua_pushnil(L);
		lua_pushfstring(L, "Failed to create PDF file: %s", filename);
		return 2;
	}
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_file_t), 0);
	lpdf->pdf = pdf;
	lpdf->output_ctx = NULL;
	luaL_getmetatable(L, LUA_PDFIO_FILE_MT);
	lua_setmetatable(L, -2);
	return 1;
}

typedef struct {
	lua_State *L;
	int fn_ref;
} lua_pdfio_output_ctx_t;

static ssize_t output_cb(void *ctx, const void *data, size_t datalen) {
	lua_pdfio_output_ctx_t *octx = (lua_pdfio_output_ctx_t *)ctx;
	lua_State *L = octx->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, octx->fn_ref);
	lua_pushlstring(L, (const char *)data, datalen);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		fprintf(stderr, "lua-pdfio: output callback error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return -1;
	}
	return (ssize_t)datalen;
}

int l_file_create_output(lua_State *L) {
	luaL_checktype(L, 1, LUA_TFUNCTION);
	const char *version = luaL_optstring(L, 2, NULL);
	lua_pdfio_output_ctx_t *octx = (lua_pdfio_output_ctx_t *)malloc(sizeof(lua_pdfio_output_ctx_t));
	if (!octx) return luaL_error(L, "out of memory");
	octx->L = L;
	lua_pushvalue(L, 1);
	octx->fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pdfio_file_t *pdf = pdfioFileCreateOutput(output_cb, octx, version, NULL, NULL, error_cb, NULL);
	if (!pdf) {
		luaL_unref(L, LUA_REGISTRYINDEX, octx->fn_ref);
		free(octx);
		lua_pushnil(L);
		lua_pushstring(L, "Failed to create PDF output");
		return 2;
	}
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_file_t), 0);
	lpdf->pdf = pdf;
	lpdf->output_ctx = octx;
	luaL_getmetatable(L, LUA_PDFIO_FILE_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_temporary(lua_State *L) {
	const char *version = luaL_optstring(L, 1, NULL);
	char filename[1024];
	pdfio_file_t *pdf = pdfioFileCreateTemporary(filename, sizeof(filename), version, NULL, NULL, error_cb, NULL);
	if (!pdf) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to create temporary PDF file");
		return 2;
	}
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_file_t), 0);
	lpdf->pdf = pdf;
	lpdf->output_ctx = NULL;
	luaL_getmetatable(L, LUA_PDFIO_FILE_MT);
	lua_setmetatable(L, -2);
	lua_pushstring(L, filename);
	return 2;
}

static int l_file_close(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	if (lpdf->pdf) {
		pdfioFileClose(lpdf->pdf);
		lpdf->pdf = NULL;
	}
	if (lpdf->output_ctx) {
		lua_pdfio_output_ctx_t *octx = (lua_pdfio_output_ctx_t *)lpdf->output_ctx;
		luaL_unref(L, LUA_REGISTRYINDEX, octx->fn_ref);
		free(octx);
		lpdf->output_ctx = NULL;
	}
	return 0;
}

static int l_file_get_num_pages(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	if (!lpdf->pdf) return luaL_error(L, "attempt to use a closed PDF file");
	lua_pushinteger(L, (lua_Integer)pdfioFileGetNumPages(lpdf->pdf));
	return 1;
}

static int l_file_get_page(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t n = (size_t)luaL_checkinteger(L, 2);
	if (!lpdf->pdf) return luaL_error(L, "attempt to use a closed PDF file");
	pdfio_obj_t *obj = pdfioFileGetPage(lpdf->pdf, n);
	if (!obj) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_page(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_testudata(L, 2, LUA_PDFIO_DICT_MT);
	if (!lpdf->pdf) return luaL_error(L, "attempt to use a closed PDF file");
	pdfio_stream_t *stream = pdfioFileCreatePage(lpdf->pdf, ldict ? ldict->dict : NULL);
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

static int l_file_create_font_obj_from_base(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *name = luaL_checkstring(L, 2);
	pdfio_obj_t *obj = pdfioFileCreateFontObjFromBase(lpdf->pdf, name);
	if (!obj) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_font_obj_from_data(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t datasize;
	const void *data = (const void *)luaL_checklstring(L, 2, &datasize);
	bool unicode = lua_toboolean(L, 3);
	pdfio_obj_t *obj = pdfioFileCreateFontObjFromData(lpdf->pdf, data, datasize, unicode);
	if (!obj) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_image_obj_from_file(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *filename = luaL_checkstring(L, 2);
	bool interpolate = lua_toboolean(L, 3);
	pdfio_obj_t *obj = pdfioFileCreateImageObjFromFile(lpdf->pdf, filename, interpolate);
	if (!obj) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_image_obj_from_data(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t datalen;
	const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 2, &datalen);
	size_t width = (size_t)luaL_checkinteger(L, 3);
	size_t height = (size_t)luaL_checkinteger(L, 4);
	size_t num_colors = (size_t)luaL_checkinteger(L, 5);
	lua_pdfio_array_t *lcolordata = (lua_pdfio_array_t *)luaL_testudata(L, 6, LUA_PDFIO_ARRAY_MT);
	bool alpha = lua_toboolean(L, 7);
	bool interpolate = lua_toboolean(L, 8);
	size_t needed = width * height * (num_colors + (alpha ? 1 : 0));
	if (datalen < needed) {
		return luaL_error(L, "image data too short: need %d bytes, got %d", (int)needed, (int)datalen);
	}
	pdfio_obj_t *obj = pdfioFileCreateImageObjFromData(lpdf->pdf, data, width, height, num_colors, lcolordata ? lcolordata->array : NULL, alpha, interpolate);
	if (!obj) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_dict_create(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_dict_t *dict = pdfioDictCreate(lpdf->pdf);
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

int l_file_array_create(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_array_t *array = pdfioArrayCreate(lpdf->pdf);
	if (!array) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_array_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)luaL_checkudata(L, 2, LUA_PDFIO_ARRAY_MT);
	pdfio_obj_t *obj = pdfioFileCreateArrayObj(lpdf->pdf, larray->array);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_name_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *name = luaL_checkstring(L, 2);
	pdfio_obj_t *obj = pdfioFileCreateNameObj(lpdf->pdf, name);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_number_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	double num = luaL_checknumber(L, 2);
	pdfio_obj_t *obj = pdfioFileCreateNumberObj(lpdf->pdf, num);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_string_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *s = luaL_checkstring(L, 2);
	pdfio_obj_t *obj = pdfioFileCreateStringObj(lpdf->pdf, s);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_create_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_checkudata(L, 2, LUA_PDFIO_DICT_MT);
	pdfio_obj_t *obj = pdfioFileCreateObj(lpdf->pdf, ldict->dict);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_find_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t num = (size_t)luaL_checkinteger(L, 2);
	pdfio_obj_t *obj = pdfioFileFindObj(lpdf->pdf, num);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_copy_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_obj_t *lsrc = (lua_pdfio_obj_t *)luaL_checkudata(L, 2, LUA_PDFIO_OBJ_MT);
	pdfio_obj_t *obj = pdfioObjCopy(lpdf->pdf, lsrc->obj);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_copy_page(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_obj_t *lsrc = (lua_pdfio_obj_t *)luaL_checkudata(L, 2, LUA_PDFIO_OBJ_MT);
	lua_pushboolean(L, pdfioPageCopy(lpdf->pdf, lsrc->obj));
	return 1;
}

int l_file_get_permissions(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_encryption_t encryption;
	pdfio_permission_t permissions = pdfioFileGetPermissions(lpdf->pdf, &encryption);
	lua_pushinteger(L, (lua_Integer)permissions);
	lua_pushinteger(L, (lua_Integer)encryption);
	return 2;
}

int l_file_set_permissions(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_permission_t permissions = (pdfio_permission_t)luaL_checkinteger(L, 2);
	pdfio_encryption_t encryption = (pdfio_encryption_t)luaL_checkinteger(L, 3);
	const char *owner_password = luaL_optstring(L, 4, NULL);
	const char *user_password = luaL_optstring(L, 5, NULL);
	lua_pushboolean(L, pdfioFileSetPermissions(lpdf->pdf, permissions, encryption, owner_password, user_password));
	return 1;
}

int l_file_create_icc_obj_from_file(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *filename = luaL_checkstring(L, 2);
	size_t num_colors = (size_t)luaL_checkinteger(L, 3);
	pdfio_obj_t *obj = pdfioFileCreateICCObjFromFile(lpdf->pdf, filename, num_colors);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_icc_obj_from_data(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t datalen;
	const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 2, &datalen);
	size_t num_colors = (size_t)luaL_checkinteger(L, 3);
	pdfio_obj_t *obj = pdfioFileCreateICCObjFromData(lpdf->pdf, data, datalen, num_colors);
	if (!obj) { lua_pushnil(L); return 1; }
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
	lobj->obj = obj;
	luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_color_from_icc_obj(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)luaL_checkudata(L, 2, LUA_PDFIO_OBJ_MT);
	pdfio_array_t *array = pdfioArrayCreateColorFromICCObj(lpdf->pdf, lobj->obj);
	if (!array) { lua_pushnil(L); return 1; }
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_color_from_standard(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t num_colors = (size_t)luaL_checkinteger(L, 2);
	pdfio_cs_t cs = (pdfio_cs_t)luaL_checkinteger(L, 3);
	pdfio_array_t *array = pdfioArrayCreateColorFromStandard(lpdf->pdf, num_colors, cs);
	if (!array) { lua_pushnil(L); return 1; }
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_color_from_palette(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t num_colors = (size_t)luaL_checkinteger(L, 2);
	size_t colorslen;
	const unsigned char *colors = (const unsigned char *)luaL_checklstring(L, 3, &colorslen);
	pdfio_array_t *array = pdfioArrayCreateColorFromPalette(lpdf->pdf, num_colors, colors);
	if (!array) { lua_pushnil(L); return 1; }
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_color_from_matrix(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t num_colors = (size_t)luaL_checkinteger(L, 2);
	double gamma = luaL_checknumber(L, 3);
	double matrix[3][3], white_point[3];
	luaL_checktype(L, 4, LUA_TTABLE);
	for (int i=0; i<3; i++) {
		for (int j=0; j<3; j++) {
			lua_rawgeti(L, 4, i*3+j+1);
			matrix[i][j] = luaL_checknumber(L, -1);
			lua_pop(L, 1);
		}
	}
	luaL_checktype(L, 5, LUA_TTABLE);
	for (int i=0; i<3; i++) {
		lua_rawgeti(L, 5, i+1);
		white_point[i] = luaL_checknumber(L, -1);
		lua_pop(L, 1);
	}
	pdfio_array_t *array = pdfioArrayCreateColorFromMatrix(lpdf->pdf, num_colors, gamma, matrix, white_point);
	if (!array) { lua_pushnil(L); return 1; }
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

int l_file_create_color_from_primaries(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	size_t num_colors = (size_t)luaL_checkinteger(L, 2);
	double gamma = luaL_checknumber(L, 3);
	double wx = luaL_checknumber(L, 4);
	double wy = luaL_checknumber(L, 5);
	double rx = luaL_checknumber(L, 6);
	double ry = luaL_checknumber(L, 7);
	double gx = luaL_checknumber(L, 8);
	double gy = luaL_checknumber(L, 9);
	double bx = luaL_checknumber(L, 10);
	double by = luaL_checknumber(L, 11);
	pdfio_array_t *array = pdfioArrayCreateColorFromPrimaries(lpdf->pdf, num_colors, gamma, wx, wy, rx, ry, gx, gy, bx, by);
	if (!array) { lua_pushnil(L); return 1; }
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_get_author(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetAuthor(lpdf->pdf));
	return 1;
}

static int l_file_get_creation_date(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushinteger(L, (lua_Integer)pdfioFileGetCreationDate(lpdf->pdf));
	return 1;
}

static int l_file_get_creator(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetCreator(lpdf->pdf));
	return 1;
}

static int l_file_get_keywords(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetKeywords(lpdf->pdf));
	return 1;
}

static int l_file_get_modification_date(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushinteger(L, (lua_Integer)pdfioFileGetModificationDate(lpdf->pdf));
	return 1;
}

static int l_file_get_producer(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetProducer(lpdf->pdf));
	return 1;
}

static int l_file_get_subject(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetSubject(lpdf->pdf));
	return 1;
}

static int l_file_get_title(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetTitle(lpdf->pdf));
	return 1;
}

static int l_file_get_version(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetVersion(lpdf->pdf));
	return 1;
}

static int l_file_get_catalog(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_dict_t *dict = pdfioFileGetCatalog(lpdf->pdf);
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

static int l_file_set_creation_date(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	time_t t = (time_t)luaL_checkinteger(L, 2);
	pdfioFileSetCreationDate(lpdf->pdf, t);
	return 0;
}

static int l_file_set_modification_date(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	time_t t = (time_t)luaL_checkinteger(L, 2);
	pdfioFileSetModificationDate(lpdf->pdf, t);
	return 0;
}

static int l_file_set_author(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetAuthor(lpdf->pdf, value);
	return 0;
}

static int l_file_set_title(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetTitle(lpdf->pdf, value);
	return 0;
}

static int l_file_set_creator(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetCreator(lpdf->pdf, value);
	return 0;
}

static int l_file_set_subject(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetSubject(lpdf->pdf, value);
	return 0;
}

static int l_file_set_keywords(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetKeywords(lpdf->pdf, value);
	return 0;
}

static int l_file_get_language(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetLanguage(lpdf->pdf));
	return 1;
}

static int l_file_set_language(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *value = luaL_checkstring(L, 2);
	pdfioFileSetLanguage(lpdf->pdf, value);
	return 0;
}

static int l_file_get_id(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	pdfio_array_t *array = pdfioFileGetID(lpdf->pdf);
	if (!array) {
		lua_pushnil(L);
		return 1;
	}
	lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
	larray->array = array;
	luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
	lua_setmetatable(L, -2);
	return 1;
}

static int l_file_get_name(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	lua_pushstring(L, pdfioFileGetName(lpdf->pdf));
	return 1;
}

static int l_file_add_output_intent(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *subtype = luaL_checkstring(L, 2);
	const char *condition = luaL_optstring(L, 3, NULL);
	const char *cond_id = luaL_optstring(L, 4, NULL);
	const char *reg_name = luaL_optstring(L, 5, NULL);
	const char *info = luaL_optstring(L, 6, NULL);
	lua_pdfio_obj_t *lprofile = (lua_pdfio_obj_t *)luaL_testudata(L, 7, LUA_PDFIO_OBJ_MT);
	pdfioFileAddOutputIntent(lpdf->pdf, subtype, condition, cond_id, reg_name, info, lprofile ? lprofile->obj : NULL);
	return 0;
}

static int l_file_string_create(lua_State *L) {
	lua_pdfio_file_t *lpdf = (lua_pdfio_file_t *)luaL_checkudata(L, 1, LUA_PDFIO_FILE_MT);
	const char *s = luaL_checkstring(L, 2);
	lua_pushstring(L, pdfioStringCreate(lpdf->pdf, s));
	return 1;
}

static const struct luaL_Reg lua_pdfio_file_methods[] = {
	{"close", l_file_close},
	{"get_num_pages", l_file_get_num_pages},
	{"get_page", l_file_get_page},
	{"create_page", l_file_create_page},
	{"array_create", l_file_array_create},
	{"create_font_obj_from_base", l_file_create_font_obj_from_base},
	{"create_font_obj_from_data", l_file_create_font_obj_from_data},
	{"create_image_obj_from_file", l_file_create_image_obj_from_file},
	{"create_image_obj_from_data", l_file_create_image_obj_from_data},
	{"dict_create", l_file_dict_create},
	{"create_array_obj", l_file_create_array_obj},
	{"create_name_obj", l_file_create_name_obj},
	{"create_number_obj", l_file_create_number_obj},
	{"create_string_obj", l_file_create_string_obj},
	{"create_obj", l_file_create_obj},
	{"find_obj", l_file_find_obj},
	{"copy_obj", l_file_copy_obj},
	{"copy_page", l_file_copy_page},
	{"get_permissions", l_file_get_permissions},
	{"set_permissions", l_file_set_permissions},
	{"create_icc_obj_from_file", l_file_create_icc_obj_from_file},
	{"create_icc_obj_from_data", l_file_create_icc_obj_from_data},
	{"create_color_from_icc_obj", l_file_create_color_from_icc_obj},
	{"create_color_from_standard", l_file_create_color_from_standard},
	{"create_color_from_palette", l_file_create_color_from_palette},
	{"create_color_from_matrix", l_file_create_color_from_matrix},
	{"create_color_from_primaries", l_file_create_color_from_primaries},
	{"get_author", l_file_get_author},
	{"get_catalog", l_file_get_catalog},
	{"get_creation_date", l_file_get_creation_date},
	{"set_creation_date", l_file_set_creation_date},
	{"get_creator", l_file_get_creator},
	{"get_keywords", l_file_get_keywords},
	{"get_modification_date", l_file_get_modification_date},
	{"set_modification_date", l_file_set_modification_date},
	{"get_producer", l_file_get_producer},
	{"get_subject", l_file_get_subject},
	{"get_title", l_file_get_title},
	{"get_version", l_file_get_version},
	{"set_author", l_file_set_author},
	{"set_title", l_file_set_title},
	{"set_creator", l_file_set_creator},
	{"set_subject", l_file_set_subject},
	{"set_keywords", l_file_set_keywords},
	{"get_language", l_file_get_language},
	{"set_language", l_file_set_language},
	{"get_id", l_file_get_id},
	{"get_name", l_file_get_name},
	{"add_output_intent", l_file_add_output_intent},
	{"string_create", l_file_string_create},
	{"__gc", l_file_close},
	{NULL, NULL}
};

void lua_pdfio_register_file(lua_State *L) {
	luaL_newmetatable(L, LUA_PDFIO_FILE_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_pdfio_file_methods, 0);
	lua_pop(L, 1);
}
