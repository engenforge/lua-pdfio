local M = {}

M.TYPE_DOCUMENT = 1
M.TYPE_HEADING_1 = 10
M.TYPE_HEADING_2 = 11
M.TYPE_HEADING_3 = 12
M.TYPE_PARAGRAPH = 17

function M.parse(filename)
	local f = io.open(filename, 'r')
	if not f then
		return nil
	end

	local doc = { type = M.TYPE_DOCUMENT, children = {} }

	for line in f:lines() do
		local heading_level, heading_text = line:match('^(#+)%s*(.*)$')
		if heading_level then
			local level = #heading_level
			if level >= 1 and level <= 6 then
				table.insert(doc.children, {
					type = M.TYPE_HEADING_1 + level - 1,
					text = heading_text,
					children = {},
				})
			end
		elseif line:match('%S') then
			-- Simple paragraph (no line wrapping support yet)
			table.insert(doc.children, {
				type = M.TYPE_PARAGRAPH,
				text = line,
				children = {},
			})
		end
	end

	f:close()
	return doc
end

return M
