local pdfio = require('lua-pdfio')
local markdown = require('example.markdown')

local PAGE_WIDTH = 210 * 72 / 25.4
local PAGE_HEIGHT = 11 * 72
local MARGIN_LEFT = 0.5 * 72
local MARGIN_RIGHT = PAGE_WIDTH - MARGIN_LEFT
local MARGIN_TOP = PAGE_HEIGHT - 72
local MARGIN_BOTTOM = 72

local current_y = MARGIN_TOP

local function main()
	if #arg < 2 then
		io.stderr:write('Usage: lua lua-md2pdf.lua FILENAME.md FILENAME.pdf\n')
		os.exit(1)
	end

	local md_file = arg[1]
	local pdf_file = arg[2]

	local doc = markdown.parse(md_file)
	if not doc then
		io.stderr:write('Failed to parse markdown\n')
		os.exit(1)
	end

	local pdf, err = pdfio.create(pdf_file, '1.7')
	if not pdf then
		os.exit(1)
	end

	local f_reg = pdf:create_font_obj_from_base('Helvetica')
	local f_bold = pdf:create_font_obj_from_base('Helvetica-Bold')

	local dict = pdf:dict_create()
	dict:add_font('H', f_reg)
	dict:add_font('HB', f_bold)

	local stream = pdf:create_page(dict)

	local function render_node(node)
		if
			node.type >= markdown.TYPE_HEADING_1
			and node.type <= markdown.TYPE_HEADING_1 + 5
		then
			local level = node.type - markdown.TYPE_HEADING_1
			local size = 18 - level * 2
			current_y = current_y - size * 1.5
			stream:text_begin()
			stream:set_text_font('HB', size)
			stream:text_move_to(MARGIN_LEFT, current_y)
			stream:text_show(false, node.text or '')
			stream:text_end()
			current_y = current_y - size * 0.5
		elseif node.type == markdown.TYPE_PARAGRAPH then
			local size = 11
			current_y = current_y - size * 1.4
			stream:text_begin()
			stream:set_text_font('H', size)
			stream:text_move_to(MARGIN_LEFT, current_y)
			stream:text_show(false, node.text or '')
			stream:text_end()
			current_y = current_y - size * 0.6
		end

		for _, child in ipairs(node.children or {}) do
			render_node(child)
		end
	end

	render_node(doc)

	stream:close()
	pdf:close()
	print('Created ' .. pdf_file)
end

main()
