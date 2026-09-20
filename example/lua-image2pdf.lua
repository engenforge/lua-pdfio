local pdfio = require('lua-pdfio')

local function create_pdf_image_file(imagename, pdfname, caption)
	-- Default the caption...
	if not caption then
		caption = imagename:match('([^/]+)$') or imagename
	end

	-- Create the PDF file...
	local pdf, err = pdfio.create(pdfname, '1.7')
	if not pdf then
		io.stderr:write('Failed to create PDF: ' .. tostring(err) .. '\n')
		return false
	end

	-- Create a Courier base font for the caption
	local font = pdf:create_font_obj_from_base('Courier')
	if not font then
		pdf:close()
		return false
	end

	-- Create an image object from the JPEG/PNG image file...
	local image = pdf:create_image_obj_from_file(imagename, true)
	if not image then
		pdf:close()
		return false
	end

	-- Create a page dictionary with the font and image...
	local dict = pdf:dict_create()
	dict:add_font('F1', font)
	dict:add_image('IM1', image)

	-- Create the page and its content stream...
	local page = pdf:create_page(dict)

	-- Position and scale the image on the page...
	local width = image:image_get_width()
	local height = image:image_get_height()

	local swidth = 595.28 - 72.0
	local sheight = swidth * height / width
	if sheight > (792.0 - 36.0 - 72.0) then
		sheight = 792.0 - 36.0 - 72.0
		swidth = sheight * width / height
	end

	local tx = 0.5 * (595.28 - swidth)
	local ty = 0.5 * (792.0 - 36.0 - sheight)

	page:draw_image('IM1', tx, ty + 36.0, swidth, sheight)

	-- Draw the caption in black...
	page:set_fill_color_device_gray(0.0)

	-- Compute the starting point for the text
	tx = 0.5 * (595.28 - 18.0 * 0.6 * #caption)

	-- Position and draw the caption underneath...
	page:text_begin()
	page:set_text_font('F1', 18.0)
	page:text_move_to(tx, ty)
	page:text_show(false, caption)
	page:text_end()

	-- Close and save
	page:close()
	pdf:close()

	return true
end

local function main()
	if #arg < 2 then
		io.stderr:write(
			'Usage: lua lua-image2pdf.lua FILENAME.{jpg,png} FILENAME.pdf ["TEXT"]\n'
		)
		os.exit(1)
	end

	local imagefile = arg[1]
	local pdffile = arg[2]
	local caption = arg[3]

	if create_pdf_image_file(imagefile, pdffile, caption) then
		print('Successfully created ' .. pdffile)
	else
		os.exit(1)
	end
end

main()
