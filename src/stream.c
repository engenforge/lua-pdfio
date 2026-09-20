#include "lua-pdfio.h"

static void lua_to_matrix(lua_State *L, int index, pdfio_matrix_t m) {
	luaL_checktype(L, index, LUA_TTABLE);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			lua_rawgeti(L, index, i * 2 + j + 1);
			m[i][j] = luaL_checknumber(L, -1);
			lua_pop(L, 1);
		}
	}
}

static int l_stream_read(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	size_t amt = (size_t)luaL_optinteger(L, 2, 4096);
	char *buffer = (char *)malloc(amt);
	if (!buffer) return luaL_error(L, "out of memory");
	ssize_t bytes = pdfioStreamRead(lstream->stream, buffer, amt);
	if (bytes <= 0) {
		free(buffer);
		lua_pushnil(L);
		return 1;
	}
	lua_pushlstring(L, buffer, (size_t)bytes);
	free(buffer);
	return 1;
}

static int l_stream_write(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	size_t len;
	const char *data = luaL_checklstring(L, 2, &len);
	if (pdfioStreamWrite(lstream->stream, data, len)) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int l_stream_get_token(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	char buffer[1024];
	if (pdfioStreamGetToken(lstream->stream, buffer, sizeof(buffer))) {
		lua_pushstring(L, buffer);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static int l_stream_peek(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	size_t amt = (size_t)luaL_optinteger(L, 2, 4096);
	char *buffer = (char *)malloc(amt);
	if (!buffer) return luaL_error(L, "out of memory");
	ssize_t bytes = pdfioStreamPeek(lstream->stream, buffer, amt);
	if (bytes <= 0) {
		free(buffer);
		lua_pushnil(L);
		return 1;
	}
	lua_pushlstring(L, buffer, (size_t)bytes);
	free(buffer);
	return 1;
}

static int l_stream_consume(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	size_t bytes = (size_t)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioStreamConsume(lstream->stream, bytes));
	return 1;
}

static int l_stream_puts(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *s = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioStreamPuts(lstream->stream, s));
	return 1;
}

static int l_stream_putchar(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	int ch = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioStreamPutChar(lstream->stream, ch));
	return 1;
}

static int l_stream_close(lua_State *L) {
	lua_pdfio_stream_t *lstream = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	if (lstream->stream) {
		pdfioStreamClose(lstream->stream);
		lstream->stream = NULL;
	}
	return 0;
}

static int l_content_draw_image(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	lua_pushboolean(L, pdfioContentDrawImage(lst->stream, name, x, y, w, h));
	return 1;
}

static int l_content_set_fill_color_device_gray(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double g = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetFillColorDeviceGray(lst->stream, g));
	return 1;
}

static int l_content_set_fill_color_device_rgb(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double r = luaL_checknumber(L, 2);
	double g = luaL_checknumber(L, 3);
	double b = luaL_checknumber(L, 4);
	lua_pushboolean(L, pdfioContentSetFillColorDeviceRGB(lst->stream, r, g, b));
	return 1;
}

static int l_content_set_fill_color_device_cmyk(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double c = luaL_checknumber(L, 2);
	double m = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double k = luaL_checknumber(L, 5);
	lua_pushboolean(L, pdfioContentSetFillColorDeviceCMYK(lst->stream, c, m, y, k));
	return 1;
}

static int l_content_set_fill_color_space(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *name = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioContentSetFillColorSpace(lst->stream, name));
	return 1;
}

static int l_content_set_stroke_color_device_gray(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double g = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetStrokeColorDeviceGray(lst->stream, g));
	return 1;
}

static int l_content_set_stroke_color_device_rgb(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double r = luaL_checknumber(L, 2);
	double g = luaL_checknumber(L, 3);
	double b = luaL_checknumber(L, 4);
	lua_pushboolean(L, pdfioContentSetStrokeColorDeviceRGB(lst->stream, r, g, b));
	return 1;
}

static int l_content_set_stroke_color_device_cmyk(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double c = luaL_checknumber(L, 2);
	double m = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double k = luaL_checknumber(L, 5);
	lua_pushboolean(L, pdfioContentSetStrokeColorDeviceCMYK(lst->stream, c, m, y, k));
	return 1;
}

static int l_content_set_stroke_color_space(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *name = luaL_checkstring(L, 2);
	lua_pushboolean(L, pdfioContentSetStrokeColorSpace(lst->stream, name));
	return 1;
}

static int l_content_fill_and_stroke(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	bool even_odd = lua_toboolean(L, 2);
	lua_pushboolean(L, pdfioContentFillAndStroke(lst->stream, even_odd));
	return 1;
}

static int l_content_begin_marked(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *tag = luaL_checkstring(L, 2);
	lua_pdfio_dict_t *ldict = (lua_pdfio_dict_t *)luaL_testudata(L, 3, LUA_PDFIO_DICT_MT);
	lua_pushboolean(L, pdfioContentBeginMarked(lst->stream, tag, ldict ? ldict->dict : NULL));
	return 1;
}

static int l_content_end_marked(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentEndMarked(lst->stream));
	return 1;
}

static int l_content_text_begin(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentTextBegin(lst->stream));
	return 1;
}

static int l_content_text_end(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentTextEnd(lst->stream));
	return 1;
}

static int l_content_set_text_font(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	const char *name = luaL_checkstring(L, 2);
	double size = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentSetTextFont(lst->stream, name, size));
	return 1;
}

static int l_content_text_move_to(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double tx = luaL_checknumber(L, 2);
	double ty = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentTextMoveTo(lst->stream, tx, ty));
	return 1;
}

static int l_content_text_show(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	bool unicode = lua_toboolean(L, 2);
	const char *s = luaL_checkstring(L, 3);
	lua_pushboolean(L, pdfioContentTextShow(lst->stream, unicode, s));
	return 1;
}

static int l_content_text_show_justified(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	bool unicode = lua_toboolean(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	size_t num_fragments = lua_rawlen(L, 3);
	double *offsets = malloc(num_fragments * sizeof(double));
	const char **fragments = malloc(num_fragments * sizeof(char *));

	for (size_t i = 0; i < num_fragments; i++) {
		lua_rawgeti(L, 3, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "offset");
		offsets[i] = luaL_checknumber(L, -1);
		lua_getfield(L, -2, "text");
		fragments[i] = luaL_checkstring(L, -1);
		lua_pop(L, 3);
	}

	bool res = pdfioContentTextShowJustified(lst->stream, unicode, num_fragments, offsets, fragments);
	free(offsets);
	free(fragments);
	lua_pushboolean(L, res);
	return 1;
}

static int l_content_set_text_character_spacing(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double spacing = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetTextCharacterSpacing(lst->stream, spacing));
	return 1;
}

static int l_content_set_text_leading(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double leading = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetTextLeading(lst->stream, leading));
	return 1;
}

static int l_content_set_text_rendering_mode(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	int mode = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioContentSetTextRenderingMode(lst->stream, (pdfio_textrendering_t)mode));
	return 1;
}

static int l_content_set_text_rise(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double rise = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetTextRise(lst->stream, rise));
	return 1;
}

static int l_content_set_text_matrix(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	pdfio_matrix_t m;
	lua_to_matrix(L, 2, m);
	lua_pushboolean(L, pdfioContentSetTextMatrix(lst->stream, m));
	return 1;
}

static int l_content_set_text_word_spacing(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double spacing = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetTextWordSpacing(lst->stream, spacing));
	return 1;
}

static int l_content_set_text_x_scaling(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double percent = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetTextXScaling(lst->stream, percent));
	return 1;
}

static int l_content_text_move_line(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double tx = luaL_checknumber(L, 2);
	double ty = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentTextMoveLine(lst->stream, tx, ty));
	return 1;
}

static int l_content_text_new_line(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentTextNewLine(lst->stream));
	return 1;
}

static int l_content_text_new_line_show(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double ws = luaL_checknumber(L, 2);
	double cs = luaL_checknumber(L, 3);
	bool unicode = lua_toboolean(L, 4);
	const char *s = luaL_checkstring(L, 5);
	lua_pushboolean(L, pdfioContentTextNewLineShow(lst->stream, ws, cs, unicode, s));
	return 1;
}

static int l_content_text_next_line(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentTextNextLine(lst->stream));
	return 1;
}

static int l_content_path_rect(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	double w = luaL_checknumber(L, 4);
	double h = luaL_checknumber(L, 5);
	lua_pushboolean(L, pdfioContentPathRect(lst->stream, x, y, w, h));
	return 1;
}

static int l_content_path_move_to(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentPathMoveTo(lst->stream, x, y));
	return 1;
}

static int l_content_path_line_to(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentPathLineTo(lst->stream, x, y));
	return 1;
}

static int l_content_path_curve(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x1 = luaL_checknumber(L, 2);
	double y1 = luaL_checknumber(L, 3);
	double x2 = luaL_checknumber(L, 4);
	double y2 = luaL_checknumber(L, 5);
	double x3 = luaL_checknumber(L, 6);
	double y3 = luaL_checknumber(L, 7);
	lua_pushboolean(L, pdfioContentPathCurve(lst->stream, x1, y1, x2, y2, x3, y3));
	return 1;
}

static int l_content_path_curve13(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x1 = luaL_checknumber(L, 2);
	double y1 = luaL_checknumber(L, 3);
	double x3 = luaL_checknumber(L, 4);
	double y3 = luaL_checknumber(L, 5);
	lua_pushboolean(L, pdfioContentPathCurve13(lst->stream, x1, y1, x3, y3));
	return 1;
}

static int l_content_path_curve23(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double x2 = luaL_checknumber(L, 2);
	double y2 = luaL_checknumber(L, 3);
	double x3 = luaL_checknumber(L, 4);
	double y3 = luaL_checknumber(L, 5);
	lua_pushboolean(L, pdfioContentPathCurve23(lst->stream, x2, y2, x3, y3));
	return 1;
}

static int l_content_path_close(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentPathClose(lst->stream));
	return 1;
}

static int l_content_path_end(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentPathEnd(lst->stream));
	return 1;
}

static int l_content_save(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentSave(lst->stream));
	return 1;
}

static int l_content_restore(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentRestore(lst->stream));
	return 1;
}

static int l_content_set_dash_pattern(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double phase = luaL_checknumber(L, 2);
	double on = luaL_checknumber(L, 3);
	double off = luaL_checknumber(L, 4);
	lua_pushboolean(L, pdfioContentSetDashPattern(lst->stream, phase, on, off));
	return 1;
}

static int l_content_set_line_cap(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	int lc = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioContentSetLineCap(lst->stream, (pdfio_linecap_t)lc));
	return 1;
}

static int l_content_set_line_join(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	int lj = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, pdfioContentSetLineJoin(lst->stream, (pdfio_linejoin_t)lj));
	return 1;
}

static int l_content_set_line_width(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double w = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetLineWidth(lst->stream, w));
	return 1;
}

static int l_content_set_miter_limit(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double limit = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetMiterLimit(lst->stream, limit));
	return 1;
}

static int l_content_set_flatness(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double f = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentSetFlatness(lst->stream, f));
	return 1;
}

static int l_content_stroke(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	lua_pushboolean(L, pdfioContentStroke(lst->stream));
	return 1;
}

static int l_content_fill(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	bool even_odd = lua_toboolean(L, 2);
	lua_pushboolean(L, pdfioContentFill(lst->stream, even_odd));
	return 1;
}

static int l_content_matrix_concat(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	pdfio_matrix_t m;
	lua_to_matrix(L, 2, m);
	lua_pushboolean(L, pdfioContentMatrixConcat(lst->stream, m));
	return 1;
}

static int l_content_matrix_rotate(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double degrees = luaL_checknumber(L, 2);
	lua_pushboolean(L, pdfioContentMatrixRotate(lst->stream, degrees));
	return 1;
}

static int l_content_matrix_scale(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double sx = luaL_checknumber(L, 2);
	double sy = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentMatrixScale(lst->stream, sx, sy));
	return 1;
}

static int l_content_matrix_translate(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	double tx = luaL_checknumber(L, 2);
	double ty = luaL_checknumber(L, 3);
	lua_pushboolean(L, pdfioContentMatrixTranslate(lst->stream, tx, ty));
	return 1;
}

static int l_content_clip(lua_State *L) {
	lua_pdfio_stream_t *lst = (lua_pdfio_stream_t *)luaL_checkudata(L, 1, LUA_PDFIO_STREAM_MT);
	bool even_odd = lua_toboolean(L, 2);
	lua_pushboolean(L, pdfioContentClip(lst->stream, even_odd));
	return 1;
}

static const struct luaL_Reg lua_pdfio_stream_methods[] = {
	{"read", l_stream_read},
	{"write", l_stream_write},
	{"peek", l_stream_peek},
	{"consume", l_stream_consume},
	{"puts", l_stream_puts},
	{"putchar", l_stream_putchar},
	{"get_token", l_stream_get_token},
	{"close", l_stream_close},
	{"draw_image", l_content_draw_image},
	{"set_fill_color_device_gray", l_content_set_fill_color_device_gray},
	{"set_fill_color_device_rgb", l_content_set_fill_color_device_rgb},
	{"set_fill_color_device_cmyk", l_content_set_fill_color_device_cmyk},
	{"set_fill_color_space", l_content_set_fill_color_space},
	{"set_stroke_color_device_gray", l_content_set_stroke_color_device_gray},
	{"set_stroke_color_device_rgb", l_content_set_stroke_color_device_rgb},
	{"set_stroke_color_device_cmyk", l_content_set_stroke_color_device_cmyk},
	{"set_stroke_color_space", l_content_set_stroke_color_space},
	{"fill_and_stroke", l_content_fill_and_stroke},
	{"begin_marked", l_content_begin_marked},
	{"end_marked", l_content_end_marked},
	{"text_begin", l_content_text_begin},
	{"text_end", l_content_text_end},
	{"set_text_font", l_content_set_text_font},
	{"text_move_to", l_content_text_move_to},
	{"text_show", l_content_text_show},
	{"text_show_justified", l_content_text_show_justified},
	{"set_text_character_spacing", l_content_set_text_character_spacing},
	{"set_text_leading", l_content_set_text_leading},
	{"set_text_rendering_mode", l_content_set_text_rendering_mode},
	{"set_text_rise", l_content_set_text_rise},
	{"set_text_x_scaling", l_content_set_text_x_scaling},
	{"set_text_matrix", l_content_set_text_matrix},
	{"set_text_word_spacing", l_content_set_text_word_spacing},
	{"text_move_line", l_content_text_move_line},
	{"text_new_line", l_content_text_new_line},
	{"text_new_line_show", l_content_text_new_line_show},
	{"text_next_line", l_content_text_next_line},
	{"path_rect", l_content_path_rect},
	{"path_move_to", l_content_path_move_to},
	{"path_line_to", l_content_path_line_to},
	{"path_curve", l_content_path_curve},
	{"path_curve13", l_content_path_curve13},
	{"path_curve23", l_content_path_curve23},
	{"path_close", l_content_path_close},
	{"path_end", l_content_path_end},
	{"save", l_content_save},
	{"restore", l_content_restore},
	{"set_dash_pattern", l_content_set_dash_pattern},
	{"set_line_cap", l_content_set_line_cap},
	{"set_line_join", l_content_set_line_join},
	{"set_line_width", l_content_set_line_width},
	{"set_miter_limit", l_content_set_miter_limit},
	{"set_flatness", l_content_set_flatness},
	{"stroke", l_content_stroke},
	{"fill", l_content_fill},
	{"matrix_concat", l_content_matrix_concat},
	{"matrix_rotate", l_content_matrix_rotate},
	{"matrix_scale", l_content_matrix_scale},
	{"matrix_translate", l_content_matrix_translate},
	{"clip", l_content_clip},
	{"__gc", l_stream_close},
	{NULL, NULL}
};

void lua_pdfio_register_stream(lua_State *L) {
	luaL_newmetatable(L, LUA_PDFIO_STREAM_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_pdfio_stream_methods, 0);
	lua_pop(L, 1);
}
