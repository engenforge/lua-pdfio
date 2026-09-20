local M = {}

local total = 0
local passed = 0

function M.test(group, name, func)
	total = total + 1

	local ok, err = pcall(func)
	if ok then
		passed = passed + 1
		print(string.format('[PASS] %s: %s', group, name))
	else
		print(string.format('[FAIL] %s: %s (%s)', group, name, tostring(err)))
	end
end

function M.summary()
	print(string.format('\n%d tests executed, %d passed', total, passed))
	if passed < total then
		os.exit(1)
	end
end

return M
