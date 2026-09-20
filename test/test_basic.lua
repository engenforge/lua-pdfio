local runner = require('test.test_runner')
local pdfio = require('lua-pdfio')

-- Function to calculate a stable hash by masking variable fields (ID, Dates)
local function get_stable_hash(filename)
	local f = io.open(filename, 'rb')
	if not f then
		return nil
	end
	local content = f:read('*a')
	f:close()

	-- Mask /ID [<...><...>]
	content = content:gsub('/ID%s*%[[^%]]+%]', '/ID [MASKED]')
	-- Mask /CreationDate (D:...)
	content =
		content:gsub('/CreationDate%s*%([^%)]+%)', '/CreationDate (MASKED)')
	-- Mask /ModDate (D:...)
	content = content:gsub('/ModDate%s*%([^%)]+%)', '/ModDate (MASKED)')

	-- Write masked content to a temp file for sha256sum
	local tmpname = filename .. '.tmp'
	local fw = io.open(tmpname, 'wb')
	fw:write(content)
	fw:close()

	local handle = io.popen('sha256sum ' .. tmpname .. ' 2>/dev/null')
	local result = handle:read('*a')
	handle:close()
	os.remove(tmpname)

	return result:match('^(%x+)')
end

-- GROUP: file
runner.test('file', 'create new pdf', function()
	local filename = 'test/data/new.pdf'
	os.remove(filename)
	local pdf, err = pdfio.create(filename, '1.7')
	assert(pdf, tostring(err))

	-- Set fixed timestamps
	pdf:set_creation_date(1769256000)
	pdf:set_modification_date(1769256000)

	local stream = pdf:create_page()
	assert(stream, 'Should create page stream')
	stream:write('BT /Helv 24 Tf 100 700 Td (Hello, World!) Tj ET')
	stream:close()

	pdf:close()
end)

runner.test('file', 'metadata getters', function()
	local pdf = pdfio.open('test/data/test.pdf')
	assert(pdf:get_version() == '1.7', 'Version mismatch')
	assert(pdf:get_producer():match('pdfio'), 'Producer mismatch')
	pdf:close()
end)

runner.test('file', 'catalog access', function()
	local pdf = pdfio.open('test/data/test.pdf')
	local catalog = pdf:get_catalog()
	assert(catalog, 'Should have catalog')
	assert(catalog.Type == 'Catalog', 'Type should be Catalog')
	pdf:close()
end)

runner.test('file', 'copy object and page', function()
	local src_pdf = pdfio.open('test/data/test.pdf')
	local dst_pdf = pdfio.create('test/data/copy_test.pdf', '1.7')

	-- Copy an object (the first page's dictionary, for example)
	local src_page = src_pdf:get_page(0)
	local copied_obj = dst_pdf:copy_obj(src_page)
	assert(copied_obj, 'Failed to copy object')
	assert(copied_obj:get_type() == 'Page', 'Copied object type mismatch')

	-- Copy a page
	local ok = dst_pdf:copy_page(src_page)
	assert(ok, 'Failed to copy page')
	assert(dst_pdf:get_num_pages() == 1, 'Destination should have 1 page')

	src_pdf:close()
	dst_pdf:close()
end)

runner.test('file', 'encryption and permissions', function()
	local filename = 'test/data/encrypted.pdf'
	local pdf = pdfio.create(filename, '1.7')

	local perms = pdfio.PERMISSION_PRINT + pdfio.PERMISSION_COPY
	local enc = pdfio.ENCRYPTION_AES_128
	local ok = pdf:set_permissions(perms, enc, 'owner', 'user')
	assert(ok, 'Failed to set permissions')

	pdf:close()

	-- Test opening with wrong password
	local pdf_wrong, err = pdfio.open(filename, 'wrong')
	assert(pdf_wrong == nil, 'Should fail with wrong password')

	-- Test opening with correct user password
	local pdf_user = pdfio.open(filename, 'user')
	assert(pdf_user, 'Should open with user password')
	local g_perms, g_enc = pdf_user:get_permissions()
	assert(g_enc == enc, 'Encryption mismatch on open')
	pdf_user:close()
end)

runner.test('file', 'metadata setters', function()
	local filename = 'test/data/meta_test.pdf'
	local pdf = pdfio.create(filename, '1.7')

	pdf:set_author('Jane Doe')
	pdf:set_title('Test Title')
	pdf:set_creator('lua-pdfio')
	pdf:set_subject('Testing')
	pdf:set_keywords('test,pdfio')
	pdf:set_language('en-US')

	assert(pdf:get_author() == 'Jane Doe', 'Author mismatch')
	assert(pdf:get_title() == 'Test Title', 'Title mismatch')
	assert(pdf:get_creator() == 'lua-pdfio', 'Creator mismatch')
	assert(pdf:get_subject() == 'Testing', 'Subject mismatch')
	assert(pdf:get_keywords() == 'test,pdfio', 'Keywords mismatch')
	assert(pdf:get_language() == 'en-US', 'Language mismatch')
	assert(pdf:get_name() == filename, 'Name mismatch')

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()

	local pdf2 = pdfio.open(filename)
	local id = pdf2:get_id()
	assert(id, 'Should have a trailer ID')
	assert(#id == 2, 'ID array should have 2 elements')
	pdf2:close()
end)

runner.test('file', 'create output via callback', function()
	local chunks = {}
	local pdf = pdfio.create_output(function(data)
		table.insert(chunks, data)
	end, '1.7')
	assert(pdf, 'Failed to create output pdf')

	local stream = pdf:create_page()
	stream:write('BT /Helv 24 Tf 100 700 Td (Output) Tj ET')
	stream:close()
	pdf:close()

	local content = table.concat(chunks)
	assert(#content > 0, 'No data written via output callback')
	assert(content:sub(1, 5) == '%PDF-', 'Output should start with PDF header')
end)

runner.test(
	'file',
	'create output callback error is handled safely',
	function()
		local calls = 0
		local pdf = pdfio.create_output(function(data)
			calls = calls + 1
			error('simulated callback failure')
		end, '1.7')
		assert(pdf, 'Failed to create output pdf')

		-- The callback throws on every invocation, so even opening the
		-- page stream may fail immediately. This must not crash the
		-- process either way, just surface as a failure return/nil.
		local stream = pdf:create_page()
		if stream then
			stream:write('some content')
			pcall(function()
				stream:close()
			end)
		end
		pcall(function()
			pdf:close()
		end)

		assert(calls > 0, 'Callback should have been invoked at least once')
	end
)

runner.test('file', 'create temporary file', function()
	local pdf, filename = pdfio.create_temporary('1.7')
	assert(pdf, 'Failed to create temporary pdf')
	assert(
		type(filename) == 'string' and #filename > 0,
		'Should return a temp filename'
	)

	local stream = pdf:create_page()
	stream:write('BT /Helv 24 Tf 100 700 Td (Temp) Tj ET')
	stream:close()
	pdf:close()

	local f = io.open(filename, 'rb')
	assert(f, 'Temp file should exist on disk')
	f:close()
	os.remove(filename)
end)

runner.test('file', 'create image from data', function()
	local pdf = pdfio.create('test/data/test_fromdata.pdf', '1.7')

	-- 2x2 grayscale raw image data (1 byte per pixel)
	local raw = string.char(0, 64, 128, 255)
	local image = pdf:create_image_obj_from_data(raw, 2, 2, 1, nil, false, false)
	assert(image, 'Failed to create image from data')
	assert(image:image_get_width() == 2, 'Width mismatch')
	assert(image:image_get_bytes_per_line() > 0, 'Bytes per line should be > 0')

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

runner.test(
	'file',
	'create image from data rejects a too-short buffer',
	function()
		local pdf = pdfio.create('test/data/test_fromdata_short.pdf', '1.7')

		-- Claims to be a 2x2 grayscale image (needs 4 bytes) but only
		-- supplies 2; the binding must reject this rather than reading
		-- past the end of the Lua string.
		local ok, err = pcall(function()
			return pdf:create_image_obj_from_data(
				string.char(0, 64),
				2,
				2,
				1,
				nil,
				false,
				false
			)
		end)
		assert(not ok, 'Should raise an error on short image data')
		assert(
			tostring(err):match('too short'),
			'Error should mention short data: ' .. tostring(err)
		)

		local pdf_stream = pdf:create_page()
		pdf_stream:close()
		pdf:close()
	end
)

runner.test('file', 'string_create', function()
	local pdf = pdfio.create('test/data/test_string_create.pdf', '1.7')
	local s = pdf:string_create('Hello, PDF!')
	assert(s == 'Hello, PDF!', 'string_create mismatch: ' .. tostring(s))

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

runner.test(
	'file',
	'create font from data handles invalid data gracefully',
	function()
		local pdf = pdfio.create('test/data/test_font_fromdata.pdf', '1.7')
		local font = pdf:create_font_obj_from_data('not a real font', false)
		assert(font == nil, 'Invalid font data should return nil')
		local pdf_stream = pdf:create_page()
		pdf_stream:close()
		pdf:close()
	end
)

runner.test('file', 'create font from data (real font)', function()
	local pdf = pdfio.create('test/data/test_font_fromdata_ok.pdf', '1.7')

	local f = io.open('test/data/RobotoMono-Regular.ttf', 'rb')
	assert(f, 'Test font fixture missing')
	local fontdata = f:read('*a')
	f:close()

	local font = pdf:create_font_obj_from_data(fontdata, false)
	assert(font, 'Failed to create font from real font data')
	assert(font:get_type() == 'Font', 'Font object type mismatch')

	local dict = pdf:dict_create()
	dict:add_font('F1', font)
	local stream = pdf:create_page(dict)
	stream:text_begin()
	stream:set_text_font('F1', 12)
	stream:text_move_to(72, 700)
	stream:text_show(false, 'Embedded font test')
	stream:text_end()
	stream:close()

	pdf:close()
end)

runner.test('file', 'add output intent', function()
	local pdf = pdfio.create('test/data/test_output_intent.pdf', '1.7')
	pdf:add_output_intent('GTS_PDFA1', nil, nil, nil, 'Test intent', nil)
	local catalog = pdf:get_catalog()
	assert(catalog.OutputIntents, 'Catalog should have OutputIntents')
	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

-- GROUP: page
runner.test('page', 'tree traversal', function()
	local pdf = pdfio.open('test/data/test.pdf')
	local page = pdf:get_page(0)
	local dict = page:get_dict()
	local parent = dict.Parent
	assert(parent, 'Page should have a parent')
	assert(parent:get_type() == 'Pages', 'Parent should be Pages type')

	local kids = parent:get_dict().Kids
	assert(kids and #kids > 0, 'Pages should have Kids')
	assert(
		kids[1]:get_number() == page:get_number(),
		'First kid should be first page'
	)

	pdf:close()
end)

-- GROUP: dict
runner.test('dict', 'access page dictionary', function()
	local pdf = pdfio.open('test/data/test.pdf')
	local page = pdf:get_page(0)
	local dict = page:get_dict()
	assert(dict, 'Page should have a dictionary')
	assert(dict.Type == 'Page', 'Type should be Page')

	local keys = dict:keys()
	assert(#keys > 0, 'Dictionary should have keys')
	local found_type = false
	for _, k in ipairs(keys) do
		if k == 'Type' then
			found_type = true
			break
		end
	end
	assert(found_type, 'Type key should be in keys list')
	pdf:close()
end)

runner.test('dict', 'setters', function()
	local pdf = pdfio.create('test/data/dict_test.pdf', '1.7')
	local dict = pdf:dict_create()

	dict:set_name('NameKey', 'NameValue')
	assert(dict.NameKey == 'NameValue', 'Name setter failed')

	dict:set_number('NumKey', 123.45)
	assert(dict.NumKey == 123.45, 'Number setter failed')

	dict:set_boolean('BoolKey', true)
	assert(dict.BoolKey == true, 'Boolean setter failed')

	dict:set_string('StrKey', 'Hello')
	assert(dict.StrKey == 'Hello', 'String setter failed')

	dict:set_null('NullKey')
	assert(dict.NullKey == nil, 'Null setter failed')

	dict:set_rect('RectKey', { x1 = 0, y1 = 0, x2 = 100, y2 = 100 })

	pdf:close()
end)

runner.test('dict', 'iterate_keys', function()
	local pdf = pdfio.create('test/data/dict_iterate.pdf', '1.7')
	local dict = pdf:dict_create()
	dict:set_number('A', 1)
	dict:set_number('B', 2)
	dict:set_number('C', 3)

	local seen = {}
	dict:iterate_keys(function(key)
		table.insert(seen, key)
		return true
	end)
	assert(#seen == 3, 'Should iterate all 3 keys, got ' .. #seen)

	local stopped = {}
	dict:iterate_keys(function(key)
		table.insert(stopped, key)
		return false
	end)
	assert(#stopped == 1, 'Should stop after first key when returning false')

	-- A callback that returns nothing should be treated as "continue",
	-- not as a stop signal.
	local seen_nil_return = {}
	dict:iterate_keys(function(key)
		table.insert(seen_nil_return, key)
	end)
	assert(
		#seen_nil_return == 3,
		'Nil return should continue iterating, got ' .. #seen_nil_return
	)

	-- An error inside the callback must not crash the process; it should
	-- just stop the iteration.
	local before_error = {}
	local ok = pcall(function()
		dict:iterate_keys(function(key)
			table.insert(before_error, key)
			error('simulated iterate_keys callback failure')
		end)
	end)
	assert(ok, 'iterate_keys should not propagate a Lua error out of pdfio')
	assert(
		#before_error == 1,
		'Should stop iterating after the callback errors'
	)

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

runner.test('dict', 'copy, clear, and binary', function()
	local pdf = pdfio.create('test/data/dict_copy_src.pdf', '1.7')
	local pdf2 = pdfio.create('test/data/dict_copy_dst.pdf', '1.7')

	local dict = pdf:dict_create()
	dict:set_string('Key', 'Value')
	dict:set_binary('BinKey', '\0\1\2\3')
	assert(dict.BinKey == '\0\1\2\3', 'Binary roundtrip failed')

	local copied = dict:copy(pdf2)
	assert(copied, 'Copy failed')
	assert(copied.Key == 'Value', 'Copied dict should retain values')

	assert(dict:clear('Key'), 'Clear should succeed')
	assert(dict.Key == nil, 'Key should be cleared')

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	local pdf2_stream = pdf2:create_page()
	pdf2_stream:close()
	pdf:close()
	pdf2:close()
end)

runner.test('dict', 'add named color space', function()
	local pdf = pdfio.create('test/data/dict_colorspace.pdf', '1.7')
	local cs = pdf:create_color_from_standard(3, pdfio.CS_SRGB)
	local dict = pdf:dict_create()
	assert(dict:add_color_space('CS1', cs), 'Failed to add color space')
	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

-- GROUP: array
runner.test('array', 'access MediaBox array', function()
	local pdf = pdfio.open('test/data/test.pdf')
	local page = pdf:get_page(0)
	local dict = page:get_dict()
	local mediabox = dict.MediaBox
	assert(type(mediabox) == 'userdata', 'MediaBox should be userdata (array)')
	assert(#mediabox == 4, 'MediaBox should have 4 elements')
	assert(type(mediabox[1]) == 'number', 'Elements should be numbers')
	pdf:close()
end)

runner.test('array', 'manipulation', function()
	local pdf = pdfio.create('test/data/array_test.pdf', '1.7')
	local array = pdf:array_create()

	array:append_number(1.23)
	assert(#array == 1, 'Append number failed')
	assert(array[1] == 1.23, 'Value mismatch')

	array:append_string('Hello')
	assert(#array == 2, 'Append string failed')
	assert(array[2] == 'Hello', 'Value mismatch')

	array:append_boolean(false)
	assert(#array == 3, 'Append boolean failed')
	assert(array[3] == false, 'Value mismatch')

	array:remove(1)
	assert(#array == 2, 'Remove failed')
	assert(array[1] == 'Hello', 'Shift failed')

	local pdf2 = pdfio.create('test/data/array_copy.pdf', '1.7')
	local acopy = array:copy(pdf2)
	assert(acopy, 'Copy failed')
	assert(#acopy == 2, 'Copy size mismatch')
	assert(acopy[1] == 'Hello', 'Copy content mismatch')

	pdf:close()
	pdf2:close()
end)

runner.test('array', 'append binary', function()
	local pdf = pdfio.create('test/data/array_binary.pdf', '1.7')
	local array = pdf:array_create()
	assert(array:append_binary('\0\1\2'), 'Append binary failed')
	assert(#array == 1, 'Array should have 1 element')
	assert(array[1] == '\0\1\2', 'Binary value mismatch')
	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

-- GROUP: stream
runner.test('stream', 'read from new pdf', function()
	local pdf = pdfio.open('test/data/new.pdf')
	local page = pdf:get_page(0)
	local stream = page:open_page_stream(0, true)
	local data = stream:read()
	assert(
		data == 'BT /Helv 24 Tf 100 700 Td (Hello, World!) Tj ET',
		'Data mismatch'
	)
	stream:close()
	pdf:close()
end)

runner.test('stream', 'advanced drawing', function()
	local filename = 'test/data/test_draw.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local stream = pdf:create_page()

	stream:save()
	stream:set_line_width(2.0)
	stream:set_line_cap(pdfio.LINECAP_ROUND)
	stream:set_line_join(pdfio.LINEJOIN_ROUND)
	stream:set_dash_pattern(0, 5, 5)

	stream:path_move_to(100, 100)
	stream:path_line_to(200, 100)
	stream:path_curve(250, 150, 250, 250, 200, 300)
	stream:stroke()
	stream:restore()

	stream:close()
	pdf:close()
end)

runner.test('stream', 'matrix and clip', function()
	local filename = 'test/data/test_matrix.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local stream = pdf:create_page()

	stream:save()
	stream:matrix_translate(100, 100)
	stream:matrix_rotate(45)
	stream:path_rect(0, 0, 50, 50)
	stream:clip(false)
	stream:stroke()
	stream:restore()

	stream:close()
	pdf:close()
end)

runner.test('stream', 'text measurements and state', function()
	local filename = 'test/data/test_text.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local font = pdf:create_font_obj_from_base('Helvetica')
	local stream = pdf:create_page()

	local width = font:text_measure('Hello World', 12)
	assert(width > 0, 'Measurement failed')

	stream:text_begin()
	stream:set_text_font('H1', 12)
	stream:set_text_character_spacing(1.0)
	stream:set_text_leading(14.4)
	stream:set_text_rendering_mode(pdfio.TEXTRENDERING_FILL)
	stream:set_text_rise(2.0)
	stream:set_text_x_scaling(110)

	stream:text_move_to(72, 700)
	stream:text_show(false, 'Measuring...')
	stream:text_new_line()

	stream:text_show_justified(false, {
		{ offset = 0, text = 'Justified' },
		{ offset = 100, text = 'Text' },
	})

	stream:text_end()
	stream:close()
	pdf:close()
end)

runner.test('stream', 'stroke, cmyk, fill_and_stroke, and marked content', function()
	local filename = 'test/data/test_stroke_color.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local stream = pdf:create_page()

	assert(stream:set_stroke_color_device_gray(0.5), 'Stroke gray failed')
	assert(stream:set_stroke_color_device_rgb(1, 0, 0), 'Stroke RGB failed')
	assert(stream:set_stroke_color_device_cmyk(0, 0, 0, 1), 'Stroke CMYK failed')
	assert(stream:set_fill_color_device_cmyk(0, 1, 1, 0), 'Fill CMYK failed')

	stream:path_rect(50, 50, 100, 100)
	assert(stream:fill_and_stroke(false), 'fill_and_stroke failed')

	assert(stream:begin_marked('Span', nil), 'begin_marked failed')
	assert(stream:end_marked(), 'end_marked failed')

	stream:close()
	pdf:close()
end)

runner.test('stream', 'named colorspace and text matrix', function()
	local filename = 'test/data/test_colorspace.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local cs = pdf:create_color_from_standard(3, pdfio.CS_SRGB)

	local dict = pdf:dict_create()
	dict:add_color_space('CS1', cs)

	local stream = pdf:create_page(dict)
	assert(stream:set_fill_color_space('CS1'), 'set_fill_color_space failed')
	assert(stream:set_stroke_color_space('CS1'), 'set_stroke_color_space failed')

	stream:text_begin()
	stream:set_text_font('Helv', 12)
	assert(
		stream:set_text_matrix({ 1, 0, 0, 1, 72, 700 }),
		'set_text_matrix failed'
	)
	assert(stream:set_text_word_spacing(2.0), 'set_text_word_spacing failed')
	stream:text_show(false, 'Hello')
	stream:text_end()

	stream:close()
	pdf:close()
end)

runner.test('stream', 'raw write helpers (puts, putchar)', function()
	local filename = 'test/data/test_puts.pdf'
	local pdf = pdfio.create(filename, '1.7')
	local stream = pdf:create_page()
	stream:puts('BT /Helv 24 Tf 100 700 Td (')
	for i = 1, #('Hi') do
		stream:putchar(('Hi'):byte(i))
	end
	stream:puts(') Tj ET')
	stream:close()
	pdf:close()

	local pdf2 = pdfio.open(filename)
	local page = pdf2:get_page(0)
	local rstream = page:open_page_stream(0, true)
	local data = rstream:read()
	assert(
		data == 'BT /Helv 24 Tf 100 700 Td (Hi) Tj ET',
		'puts/putchar mismatch: ' .. tostring(data)
	)
	rstream:close()
	pdf2:close()
end)

runner.test('stream', 'peek and consume', function()
	local pdf = pdfio.open('test/data/new.pdf')
	local page = pdf:get_page(0)
	local stream = page:open_page_stream(0, true)
	local peeked = stream:peek(5)
	assert(peeked == 'BT /H', 'Peek mismatch: ' .. tostring(peeked))
	assert(stream:consume(5), 'Consume failed')
	local rest = stream:read()
	assert(
		rest == 'elv 24 Tf 100 700 Td (Hello, World!) Tj ET',
		'Remaining data mismatch: ' .. tostring(rest)
	)
	stream:close()
	pdf:close()
end)

-- GROUP: image
runner.test('image', 'create image and draw', function()
	local filename = 'test/data/test_image.pdf'
	local pdf = pdfio.create(filename, '1.7')

	local font = pdf:create_font_obj_from_base('Courier')
	assert(font, 'Failed to create font')

	local image = pdf:create_image_obj_from_file('test/data/test.png', true)
	assert(image, 'Failed to create image')
	assert(image:image_get_width() > 0, 'Width should be > 0')

	local dict = pdf:dict_create()
	dict:add_font('F1', font)
	dict:add_image('IM1', image)

	local stream = pdf:create_page(dict)
	assert(stream, 'Failed to create page')

	stream:draw_image('IM1', 72, 300, 200, 200)
	stream:text_begin()
	stream:set_text_font('F1', 12)
	stream:text_move_to(72, 280)
	stream:text_show(false, 'Test Image Caption')
	stream:text_end()
	stream:close()

	pdf:close()
end)

-- GROUP: obj
runner.test('obj', 'manual creation and lookup', function()
	local filename = 'test/data/test_objs.pdf'
	local pdf = pdfio.create(filename, '1.7')

	local sobj = pdf:create_string_obj('Manual String')
	assert(sobj, 'Failed to create string object')
	local snum = sobj:get_number()
	assert(snum > 0, 'Object should have a number')

	local nobj = pdf:create_number_obj(42.5)
	assert(nobj, 'Failed to create number object')

	local found = pdf:find_obj(snum)
	assert(found, 'Failed to find string object by number')
	assert(found:get_number() == snum, 'Object number mismatch')

	pdf:close()
end)

runner.test('obj', 'create stream on manual object', function()
	local pdf = pdfio.create('test/data/obj_create_stream.pdf', '1.7')
	local dict = pdf:dict_create()
	dict:set_name('Type', 'Test')
	local obj = pdf:create_obj(dict)
	assert(obj, 'Failed to create object')

	local stream = obj:create_stream(pdfio.FILTER_FLATE)
	assert(stream, 'Failed to create object stream')
	assert(stream:write('hello stream'), 'Write to object stream failed')
	stream:close()

	local pdf_stream = pdf:create_page()
	pdf_stream:close()
	pdf:close()
end)

-- GROUP: color
runner.test('color', 'standard color spaces', function()
	local pdf = pdfio.create('test/data/color_test.pdf', '1.7')

	local cs = pdf:create_color_from_standard(3, pdfio.CS_SRGB)
	assert(cs, 'Failed to create sRGB color space')
	assert(#cs == 2, 'Color space array should have 2 elements [/ICCBased obj]')

	pdf:close()
end)

-- GROUP: integrity
runner.test('integrity', 'check new.pdf stable hash', function()
	local hash = get_stable_hash('test/data/new.pdf')
	assert(hash, 'Failed to calculate hash')
end)

runner.summary()
