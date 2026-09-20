local pdfio = require('lua-pdfio')

-- Unicode mapping for standard PDF character names
local unicode_map = {
	A = 0x0041,
	B = 0x0042,
	C = 0x0043,
	D = 0x0044,
	E = 0x0045,
	F = 0x0046,
	G = 0x0047,
	H = 0x0048,
	I = 0x0049,
	J = 0x004a,
	K = 0x004b,
	L = 0x004c,
	M = 0x004d,
	N = 0x004e,
	O = 0x004f,
	P = 0x0050,
	Q = 0x0051,
	R = 0x0052,
	S = 0x0053,
	T = 0x0054,
	U = 0x0055,
	V = 0x0056,
	W = 0x0057,
	X = 0x0058,
	Y = 0x0059,
	Z = 0x005a,
	a = 0x0061,
	b = 0x0062,
	c = 0x0063,
	d = 0x0064,
	e = 0x0065,
	f = 0x0066,
	g = 0x0067,
	h = 0x0068,
	i = 0x0069,
	j = 0x006a,
	k = 0x006b,
	l = 0x006c,
	m = 0x006d,
	n = 0x006e,
	o = 0x006f,
	p = 0x0070,
	q = 0x0071,
	r = 0x0072,
	s = 0x0073,
	t = 0x0074,
	u = 0x0075,
	v = 0x0076,
	w = 0x0077,
	x = 0x0078,
	y = 0x0079,
	z = 0x007a,
	-- Add more as needed or load from a more complete table
}

-- WinAnsi chars 128-159
local win_ansi = {
	[128] = 0x20AC,
	[130] = 0x201A,
	[131] = 0x0192,
	[132] = 0x201E,
	[133] = 0x2026,
	[134] = 0x2020,
	[135] = 0x2021,
	[136] = 0x02C6,
	[137] = 0x2030,
	[138] = 0x0160,
	[139] = 0x2039,
	[140] = 0x0152,
	[142] = 0x017D,
	[145] = 0x2018,
	[146] = 0x2019,
	[147] = 0x201C,
	[148] = 0x201D,
	[149] = 0x2022,
	[150] = 0x2013,
	[151] = 0x2014,
	[152] = 0x02DC,
	[153] = 0x2122,
	[154] = 0x0161,
	[155] = 0x203A,
	[156] = 0x0153,
	[158] = 0x017E,
	[159] = 0x0178,
}

local function put_utf8(ch)
	if not ch then
		return
	end
	if ch < 0x80 then
		io.write(string.char(ch))
	elseif ch < 0x800 then
		io.write(string.char(0xc0 | (ch >> 6), 0x80 | (ch & 0x3f)))
	elseif ch < 0x10000 then
		io.write(
			string.char(
				0xe0 | (ch >> 12),
				0x80 | ((ch >> 6) & 0x3f),
				0x80 | (ch & 0x3f)
			)
		)
	else
		-- 4-byte UTF-8
		io.write(
			string.char(
				0xf0 | (ch >> 18),
				0x80 | ((ch >> 12) & 0x3f),
				0x80 | ((ch >> 6) & 0x3f),
				0x80 | (ch & 0x3f)
			)
		)
	end
end

local function puts_utf16(s)
	-- Hex string to UTF-8
	local i = 1
	while i <= #s do
		local hex = s:sub(i, i + 3)
		if #hex == 4 then
			local ch = tonumber(hex, 16)
			if ch then
				-- Handle surrogate pairs if any
				if ch >= 0xd800 and ch <= 0xdbff then
					local next_hex = s:sub(i + 4, i + 7)
					if #next_hex == 4 then
						local lch = tonumber(next_hex, 16)
						if lch and lch >= 0xdc00 and lch <= 0xdfff then
							ch = (((ch & 0x3ff) << 10) | (lch & 0x3ff))
								+ 0x10000
							i = i + 4
						end
					end
				end
				put_utf8(ch)
			end
		end
		i = i + 4
	end
end

local function load_encoding(page_obj, name)
	local encoding = {}
	for k = 0, 255 do
		if k >= 128 and k <= 159 then
			encoding[k] = win_ansi[k] or k
		else
			encoding[k] = k
		end
	end

	local page_dict = page_obj:get_dict()
	if not page_dict or not page_dict.Resources then
		return encoding
	end

	local res = page_dict.Resources
	if type(res) == 'userdata' and res.get_dict then
		res = res:get_dict()
	end

	local font_dict = res.Font
	if not font_dict then
		return encoding
	end
	if type(font_dict) == 'userdata' and font_dict.get_dict then
		font_dict = font_dict:get_dict()
	end

	local font_obj = font_dict[name]
	if not font_obj or type(font_obj) ~= 'userdata' then
		return encoding
	end

	local f_dict = font_obj:get_dict()
	if not f_dict then
		return encoding
	end

	local enc = f_dict.Encoding
	if not enc then
		return encoding
	end

	local enc_dict = enc
	if type(enc) == 'userdata' and enc.get_dict then
		enc_dict = enc:get_dict()
	end

	if type(enc_dict) == 'string' then
		-- Standard encodings: MacRomanEncoding, etc.
		-- (Simplified: assume WinAnsi if not MacRoman)
		return encoding
	elseif type(enc_dict) == 'userdata' then
		local diffs = enc_dict.Differences
		if diffs then
			local idx = 0
			for k = 1, #diffs do
				local val = diffs[k]
				if type(val) == 'number' then
					idx = math.floor(val)
				elseif type(val) == 'string' then
					if idx >= 0 and idx <= 255 then
						encoding[idx] = unicode_map[val] or idx
					end
					idx = idx + 1
				end
			end
		end
	end

	return encoding
end

local function main()
	if #arg < 1 then
		io.stderr:write('Usage: lua lua-pdf2text.lua FILENAME.pdf\n')
		os.exit(1)
	end

	local pdf, err = pdfio.open(arg[1])
	if not pdf then
		io.stderr:write(
			string.format('lua-pdf2text: %s: %s\n', arg[1], tostring(err))
		)
		os.exit(1)
	end

	for i = 0, pdf:get_num_pages() - 1 do
		local page = pdf:get_page(i)
		if page then
			local encoding = load_encoding(page, '')
			local font_name = ''
			local first = true
			local in_array = false

			for j = 0, page:get_num_streams() - 1 do
				local stream = page:open_page_stream(j, true)
				if stream then
					while true do
						local token = stream:get_token()
						if not token then
							break
						end

						if token == '[' then
							in_array = true
						elseif token == ']' then
							in_array = false
						elseif
							not first
							and in_array
							and tonumber(token)
							and math.abs(tonumber(token)) > 100
						then
							io.write(' ')
						elseif token:sub(1, 1) == '(' then
							-- 8-bit text
							first = false
							local text = token:match('^%((.*)%)$')
								or token:sub(2)
							for k = 1, #text do
								local b = text:byte(k)
								put_utf8(encoding[b])
							end
						elseif token:sub(1, 1) == '<' then
							-- UTF-16 hex text
							first = false
							local hex = token:match('^<(.*)>$') or token:sub(2)
							puts_utf16(hex)
						elseif token:sub(1, 1) == '/' then
							font_name = token:sub(2)
						elseif token == 'Tf' and font_name ~= '' then
							encoding = load_encoding(page, font_name)
						elseif
							not first
							and (
								token == 'Td'
								or token == 'TD'
								or token == 'T*'
								or token == "'"
								or token == '"'
							)
						then
							io.write('\n')
							first = true
						end
					end
					if not first then
						io.write('\n')
						first = true
					end
					stream:close()
				end
			end
		end
	end

	pdf:close()
end

main()
