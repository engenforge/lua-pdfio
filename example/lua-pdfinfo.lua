local pdfio = require('lua-pdfio')

local function format_date(t)
	if t == 0 then
		return '-- not set --'
	end
	-- Lua's %c is locale-dependent like C's strftime %c
	return os.date('%c', t)
end

local function main()
	if #arg < 1 then
		io.stderr:write('Usage: lua lua-pdfinfo.lua FILENAME.pdf\n')
		os.exit(1)
	end

	local filename = arg[1]
	local pdf, err = pdfio.open(filename)
	if not pdf then
		os.exit(1)
	end

	local catalog = pdf:get_catalog()
	local author = pdf:get_author()
	local creator = pdf:get_creator()
	local has_acroform = catalog and catalog.AcroForm ~= nil
	local num_pages = pdf:get_num_pages()
	local producer = pdf:get_producer()
	local title = pdf:get_title()

	print(string.format('%s:', filename))
	print(string.format('           Title: %s', title or '-- not set --'))
	print(string.format('          Author: %s', author or '-- not set --'))
	print(string.format('         Creator: %s', creator or '-- not set --'))
	print(string.format('        Producer: %s', producer or '-- not set --'))
	print(
		string.format(
			'      Created On: %s',
			format_date(pdf:get_creation_date())
		)
	)
	print(
		string.format(
			'     Modified On: %s',
			format_date(pdf:get_modification_date())
		)
	)
	print(string.format('         Version: %s', pdf:get_version() or 'unknown'))
	print(string.format('        AcroForm: %s', has_acroform and 'Yes' or 'No'))
	print(string.format(' Number of Pages: %d', num_pages))

	-- Report the MediaBox for all of the pages
	local prev_box = { x1 = 0, y1 = 0, x2 = 0, y2 = 0 }
	local prev = 0

	for cur = 0, num_pages - 1 do
		local cur_box = { x1 = 0, y1 = 0, x2 = 0, y2 = 0 }
		local page = pdf:get_page(cur)

		-- Walk up page tree for MediaBox if not found
		while page do
			local box = page:get_rect('MediaBox')
			if box then
				cur_box = box
				break
			end
			local dict = page:get_dict()
			page = dict and dict.Parent
		end

		if
			cur == 0
			or math.abs(cur_box.x1 - prev_box.x1) > 0.01
			or math.abs(cur_box.y1 - prev_box.y1) > 0.01
			or math.abs(cur_box.x2 - prev_box.x2) > 0.01
			or math.abs(cur_box.y2 - prev_box.y2) > 0.01
		then
			if cur > prev then
				local range_text = string.format('Pages %d-%d', prev + 1, cur)
				print(
					string.format(
						'%16s: [%g %g %g %g]',
						range_text,
						prev_box.x1,
						prev_box.y1,
						prev_box.x2,
						prev_box.y2
					)
				)
			end

			prev = cur
			prev_box = cur_box
		end
	end

	if num_pages > prev then
		local range_text = string.format('Pages %d-%d', prev + 1, num_pages)
		print(
			string.format(
				'%16s: [%g %g %g %g]',
				range_text,
				prev_box.x1,
				prev_box.y1,
				prev_box.x2,
				prev_box.y2
			)
		)
	end

	pdf:close()
end

main()
