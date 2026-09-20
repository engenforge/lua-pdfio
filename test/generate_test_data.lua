local pdfio = require('lua-pdfio')

local function main()
	os.execute('mkdir -p test/data')

	local pdf_filename = 'test/data/test.pdf'
	local pdf, err = pdfio.create(pdf_filename, '1.7')
	if pdf then
		pdf:set_creation_date(1769256000)
		pdf:set_modification_date(1769256000)
		local stream = pdf:create_page()
		if stream then
			stream:write(
				'BT /Helv 24 Tf 100 700 Td (Deterministic Test PDF) Tj ET'
			)
			stream:close()
		end
		pdf:close()
		print('Generated ' .. pdf_filename)
	end
end

main()
