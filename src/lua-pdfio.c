#include "lua-pdfio.h"
#include <stdint.h>

void lua_pdfio_push_value(lua_State *L, pdfio_valtype_t type, void *value) {
	switch (type) {
		case PDFIO_VALTYPE_ARRAY: {
			lua_pdfio_array_t *larray = (lua_pdfio_array_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_array_t), 0);
			larray->array = (pdfio_array_t *)value;
			luaL_getmetatable(L, LUA_PDFIO_ARRAY_MT);
			lua_setmetatable(L, -2);
			break;
		}
		case PDFIO_VALTYPE_BOOLEAN:
			lua_pushboolean(L, (bool)(intptr_t)value);
			break;
		case PDFIO_VALTYPE_DATE:
			lua_pushinteger(L, (lua_Integer)(intptr_t)value);
			break;
		case PDFIO_VALTYPE_DICT: {
			lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_dict_t), 0);
			ldict->dict = (pdfio_dict_t *)value;
			luaL_getmetatable(L, LUA_PDFIO_DICT_MT);
			lua_setmetatable(L, -2);
			break;
		}
		case PDFIO_VALTYPE_INDIRECT: {
			lua_pdfio_obj_t *lobj = (lua_pdfio_obj_t *)lua_newuserdatauv(L, sizeof(lua_pdfio_obj_t), 0);
			lobj->obj = (pdfio_obj_t *)value;
			luaL_getmetatable(L, LUA_PDFIO_OBJ_MT);
			lua_setmetatable(L, -2);
			break;
		}
		case PDFIO_VALTYPE_NAME:
			lua_pushstring(L, (const char *)value);
			break;
		case PDFIO_VALTYPE_NULL:
			lua_pushnil(L);
			break;
		case PDFIO_VALTYPE_NUMBER:
			lua_pushnumber(L, *(double *)value);
			break;
		case PDFIO_VALTYPE_STRING:
			lua_pushstring(L, (const char *)value);
			break;
		default:
			lua_pushnil(L);
			break;
	}
}

int luaopen_pdfio(lua_State *L) {
	lua_pdfio_register_file(L);
	lua_pdfio_register_obj(L);
	lua_pdfio_register_dict(L);
	lua_pdfio_register_array(L);
	lua_pdfio_register_stream(L);

	lua_newtable(L);

	lua_pushcfunction(L, l_file_open);
	lua_setfield(L, -2, "open");
		lua_pushcfunction(L, l_file_create);
		lua_setfield(L, -2, "create");
		lua_pushcfunction(L, l_file_create_output);
		lua_setfield(L, -2, "create_output");
		lua_pushcfunction(L, l_file_create_temporary);
		lua_setfield(L, -2, "create_temporary");
		lua_pushcfunction(L, l_file_array_create);
		lua_setfield(L, -2, "array_create");
		lua_pushcfunction(L, l_file_create_icc_obj_from_file);
		lua_setfield(L, -2, "create_icc_obj_from_file");
		lua_pushcfunction(L, l_file_create_icc_obj_from_data);
		lua_setfield(L, -2, "create_icc_obj_from_data");
		lua_pushcfunction(L, l_file_create_color_from_icc_obj);
		lua_setfield(L, -2, "create_color_from_icc_obj");
		lua_pushcfunction(L, l_file_create_color_from_standard);
		lua_setfield(L, -2, "create_color_from_standard");
		lua_pushcfunction(L, l_file_create_color_from_palette);
		lua_setfield(L, -2, "create_color_from_palette");
		lua_pushcfunction(L, l_file_create_color_from_matrix);
		lua_setfield(L, -2, "create_color_from_matrix");
		lua_pushcfunction(L, l_file_create_color_from_primaries);
		lua_setfield(L, -2, "create_color_from_primaries");

		lua_pushinteger(L, PDFIO_LINECAP_BUTT); lua_setfield(L, -2, "LINECAP_BUTT");
		lua_pushinteger(L, PDFIO_LINECAP_ROUND); lua_setfield(L, -2, "LINECAP_ROUND");
		lua_pushinteger(L, PDFIO_LINECAP_SQUARE); lua_setfield(L, -2, "LINECAP_SQUARE");
		lua_pushinteger(L, PDFIO_LINEJOIN_MITER); lua_setfield(L, -2, "LINEJOIN_MITER");
		lua_pushinteger(L, PDFIO_LINEJOIN_ROUND); lua_setfield(L, -2, "LINEJOIN_ROUND");
		lua_pushinteger(L, PDFIO_LINEJOIN_BEVEL); lua_setfield(L, -2, "LINEJOIN_BEVEL");

		lua_pushinteger(L, PDFIO_TEXTRENDERING_FILL); lua_setfield(L, -2, "TEXTRENDERING_FILL");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_STROKE); lua_setfield(L, -2, "TEXTRENDERING_STROKE");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_FILL_AND_STROKE); lua_setfield(L, -2, "TEXTRENDERING_FILL_AND_STROKE");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_INVISIBLE); lua_setfield(L, -2, "TEXTRENDERING_INVISIBLE");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_FILL_PATH); lua_setfield(L, -2, "TEXTRENDERING_FILL_PATH");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_STROKE_PATH); lua_setfield(L, -2, "TEXTRENDERING_STROKE_PATH");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_FILL_AND_STROKE_PATH); lua_setfield(L, -2, "TEXTRENDERING_FILL_AND_STROKE_PATH");
		lua_pushinteger(L, PDFIO_TEXTRENDERING_TEXT_PATH); lua_setfield(L, -2, "TEXTRENDERING_TEXT_PATH");

		lua_pushinteger(L, PDFIO_ENCRYPTION_NONE); lua_setfield(L, -2, "ENCRYPTION_NONE");
		lua_pushinteger(L, PDFIO_ENCRYPTION_RC4_40); lua_setfield(L, -2, "ENCRYPTION_RC4_40");
		lua_pushinteger(L, PDFIO_ENCRYPTION_RC4_128); lua_setfield(L, -2, "ENCRYPTION_RC4_128");
		lua_pushinteger(L, PDFIO_ENCRYPTION_AES_128); lua_setfield(L, -2, "ENCRYPTION_AES_128");
		lua_pushinteger(L, PDFIO_ENCRYPTION_AES_256); lua_setfield(L, -2, "ENCRYPTION_AES_256");

		lua_pushinteger(L, PDFIO_PERMISSION_NONE); lua_setfield(L, -2, "PERMISSION_NONE");
		lua_pushinteger(L, PDFIO_PERMISSION_PRINT); lua_setfield(L, -2, "PERMISSION_PRINT");
		lua_pushinteger(L, PDFIO_PERMISSION_MODIFY); lua_setfield(L, -2, "PERMISSION_MODIFY");
		lua_pushinteger(L, PDFIO_PERMISSION_COPY); lua_setfield(L, -2, "PERMISSION_COPY");
		lua_pushinteger(L, PDFIO_PERMISSION_ANNOTATE); lua_setfield(L, -2, "PERMISSION_ANNOTATE");
		lua_pushinteger(L, PDFIO_PERMISSION_FORMS); lua_setfield(L, -2, "PERMISSION_FORMS");
		lua_pushinteger(L, PDFIO_PERMISSION_READING); lua_setfield(L, -2, "PERMISSION_READING");
		lua_pushinteger(L, PDFIO_PERMISSION_ASSEMBLE); lua_setfield(L, -2, "PERMISSION_ASSEMBLE");
		lua_pushinteger(L, PDFIO_PERMISSION_PRINT_HIGH); lua_setfield(L, -2, "PERMISSION_PRINT_HIGH");
		lua_pushinteger(L, PDFIO_PERMISSION_ALL); lua_setfield(L, -2, "PERMISSION_ALL");

		lua_pushinteger(L, PDFIO_FILTER_NONE); lua_setfield(L, -2, "FILTER_NONE");
		lua_pushinteger(L, PDFIO_FILTER_CCITTFAX); lua_setfield(L, -2, "FILTER_CCITTFAX");
		lua_pushinteger(L, PDFIO_FILTER_CRYPT); lua_setfield(L, -2, "FILTER_CRYPT");
		lua_pushinteger(L, PDFIO_FILTER_DCT); lua_setfield(L, -2, "FILTER_DCT");
		lua_pushinteger(L, PDFIO_FILTER_FLATE); lua_setfield(L, -2, "FILTER_FLATE");
		lua_pushinteger(L, PDFIO_FILTER_JBIG2); lua_setfield(L, -2, "FILTER_JBIG2");

		lua_pushinteger(L, PDFIO_CS_ADOBE); lua_setfield(L, -2, "CS_ADOBE");
		lua_pushinteger(L, PDFIO_CS_P3_D65); lua_setfield(L, -2, "CS_P3_D65");
		lua_pushinteger(L, PDFIO_CS_SRGB); lua_setfield(L, -2, "CS_SRGB");
		lua_pushinteger(L, PDFIO_CS_CGATS001); lua_setfield(L, -2, "CS_CGATS001");

		return 1;
	}
