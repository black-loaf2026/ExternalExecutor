local elevation_event = Instance.new("BindableEvent")
elevation_event.Name = "ElevationEvent"
elevation_event.Parent = game:GetService("CoreGui")
elevation_event.Event:Connect(function() task.wait(0.3) end)

for i = 1, 20 do
    elevation_event:Fire()
    task.wait(0.25)
end

print("Loading!")

local cg = game:GetService("CoreGui")
local hs = game:GetService("HttpService")
local is = game:GetService("InsertService")
local ps = game:GetService("Players")

local ExternalExecutor = Instance.new("Folder", cg)
ExternalExecutor.Name = "ExternalExecutor"
local Pointer = Instance.new("Folder", ExternalExecutor)
Pointer.Name = "Pointer"
local Bridge = Instance.new("Folder", ExternalExecutor)
Bridge.Name = "Bridge"

local plr = ps.LocalPlayer

local rtypeof = typeof

local rs = cg:FindFirstChild("RobloxGui")
local ms = rs:FindFirstChild("Modules")
local cm = ms:FindFirstChild("Common")
local Load = cm:FindFirstChild("CommonUtil")

local BridgeUrl = "http://localhost:9611"
local ProcessID = "%-PROCESS-ID-%"
local Vernushwd = "ExternalExecutor-HWID-" .. plr.UserId

local resc = 3
local function nukedata(dta, typ, set)
	local timeout = 5
	local result, clock = nil, tick()

	dta = dta or ""
	typ = typ or "none"
	set = set or {}

	hs:RequestInternal({
		Url = BridgeUrl .. "/handle",
		Body = typ .. "\n" .. ProcessID .. "\n" .. hs:JSONEncode(set) .. "\n" .. dta,
		Method = "POST",
		Headers = {
			['Content-Type'] = "text/plain",
		}
	}):Start(function(success, body)
		result = body
		result['Success'] = success
	end)

	while not result do task.wait()
		if (tick() - clock > timeout) then
			break
		end
	end

	if not result or not result.Success then
		if resc <= 0 then
			warn("[ERROR]: Server not responding!")
			return {}
		else
			resc -= 1
		end
	else
		resc = 3
	end

	return result and result.Body or ""
end

local env = getfenv(function() end)

env.getgenv = function()
	return env
end

env.identifyexecutor = function()
	return "ExternalExecutor", "1.0.1"
end
env.getexecutorname = env.identifyexecutor

env.compile = function(code : string, encoded : bool)
	local code = typeof(code) == "string" and code or ""
	local encoded = typeof(encoded) == "boolean" and encoded or false
	local res = nukedata(code, "compile", {
		["enc"] = tostring(encoded)
	})
	return res or ""
end

env.setscriptbytecode = function(script : Instance, bytecode : string)
	local obj = Instance.new("ObjectValue", Pointer)
	obj.Name = hs:GenerateGUID(false)
	obj.Value = script

	nukedata(bytecode, "setscriptbytecode", {
		["cn"] = obj.Name
	})

	obj:Destroy()
end

local clonerefs = {}
env.cloneref = function(obj)
	local proxy = newproxy(true)
	local meta = getmetatable(proxy)
	meta.__index = function(t, n)
		local v = obj[n]
		if typeof(v) == "function" then
			return function(self, ...)
				if self == t then
					self = obj
				end
				return v(self, ...)
			end
		else
			return v
		end
	end
	meta.__newindex = function(t, n, v)
		obj[n] = v
	end
	meta.__tostring = function(t)
		return tostring(obj)
	end
	meta.__metatable = getmetatable(obj)
	clonerefs[proxy] = obj
	return proxy
end

env.compareinstances = function(proxy1, proxy2)
	assert(type(proxy1) == "userdata", "Invalid argument #1 to 'compareinstances' (Instance expected, got " .. typeof(proxy1) .. ")")
	assert(type(proxy2) == "userdata", "Invalid argument #2 to 'compareinstances' (Instance expected, got " .. typeof(proxy2) .. ")")
	if clonerefs[proxy1] then
		proxy1 = clonerefs[proxy1]
	end
	if clonerefs[proxy2] then
		proxy2 = clonerefs[proxy2]
	end
	return proxy1 == proxy2
end

env.loadstring = function(code, chunkname)
	assert(type(code) == "string", "invalid argument #1 to 'loadstring' (string expected, got " .. type(code) .. ") ", 2)
	chunkname = chunkname or "loadstring"
	assert(type(chunkname) == "string", "invalid argument #2 to 'loadstring' (string expected, got " .. type(chunkname) .. ") ", 2)
	chunkname = chunkname:gsub("[^%a_]", "")
	if (code == "" or code == " ") then
		return nil, "Empty script source"
	end

	local bytecode = env.compile("return{[ [["..chunkname.."]] ]=function(...)local roe=function()return'\67\104\105\109\101\114\97\76\108\101'end;"..code.."\nend}", true)
	if #bytecode <= 1 then
		return nil, "Compile Failed!"
	end

	env.setscriptbytecode(Load, bytecode)

	local suc, res = pcall(function()
		return debug.loadmodule(Load)
	end)

	if suc then
		local suc2, res2 = pcall(function()
			return res()
		end)
		if suc2 and typeof(res2) == "table" and typeof(res2[chunkname]) == "function" then
			return setfenv(res2[chunkname], env)
		else
			return nil, "Failed To Load!"
		end
	else
		return nil, (res or "Failed To Load!")
	end
end

local supportedMethods = {"GET", "POST", "PUT", "DELETE", "PATCH"}
env.request = function(options)
	assert(type(options) == "table", "invalid argument #1 to 'request' (table expected, got " .. type(options) .. ") ", 2)
	assert(type(options.Url) == "string", "invalid option 'Url' for argument #1 to 'request' (string expected, got " .. type(options.Url) .. ") ", 2)
	options.Method = options.Method or "GET"
	options.Method = options.Method:upper()
	assert(table.find(supportedMethods, options.Method), "invalid option 'Method' for argument #1 to 'request' (a valid http method expected, got '" .. options.Method .. "') ", 2)
	assert(not (options.Method == "GET" and options.Body), "invalid option 'Body' for argument #1 to 'request' (current method is GET but option 'Body' was used)", 2)
	if options.Body then
		assert(type(options.Body) == "string", "invalid option 'Body' for argument #1 to 'request' (string expected, got " .. type(options.Body) .. ") ", 2)
		assert(pcall(function() hs:JSONDecode(options.Body) end), "invalid option 'Body' for argument #1 to 'request' (invalid json string format)", 2)
	end
	if options.Headers then assert(type(options.Headers) == "table", "invalid option 'Headers' for argument #1 to 'request' (table expected, got " .. type(options.Url) .. ") ", 2) end
	options.Body = options.Body or "{}"
	options.Headers = options.Headers or {}
	if (options.Headers["User-Agent"]) then assert(type(options.Headers["User-Agent"]) == "string", "invalid option 'User-Agent' for argument #1 to 'request.Header' (string expected, got " .. type(options.Url) .. ") ", 2) end
	options.Headers["User-Agent"] = options.Headers["User-Agent"] or "ExternalExecutor/1.0.1"
	options.Headers["ExternalExecutor-Fingerprint"] = Vernushwd
	options.Headers["Cache-Control"] = "no-cache"
	options.Headers["Roblox-Place-Id"] = tostring(game.PlaceId)
	options.Headers["Roblox-Game-Id"] = tostring(game.JobId)
	options.Headers["Roblox-Session-Id"] = hs:JSONEncode({
		["GameId"] = tostring(game.GameId),
		["PlaceId"] = tostring(game.PlaceId)
	})
	local res = nukedata("", "request", {
		['l'] = options.Url,
		['m'] = options.Method,
		['h'] = options.Headers,
		['b'] = options.Body or "{}"
	})
	if res then
		local result = hs:JSONDecode(res)
		if result['r'] ~= "OK" then
			result['r'] = "Unknown"
		end
		return {
			Success = tonumber(result['c']) and tonumber(result['c']) > 200 and tonumber(result['c']) < 300,
			StatusMessage = result['r'],
			StatusCode = tonumber(result['c']),
			Body = result['b'],
			HttpError = Enum.HttpError[result['r']],
			Headers = result['h'],
			Version = result['v']
		}
	end
	return {
		Success = false,
		StatusMessage = "Can't connect to ExternalExecutor web server!",
		StatusCode = 599;
		HttpError = Enum.HttpError.ConnectFail
	}
end

local lookupValueToCharacter = buffer.create(64)
local lookupCharacterToValue = buffer.create(256)

local alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
local padding = string.byte("=")

for index = 1, 64 do
	local value = index - 1
	local character = string.byte(alphabet, index)

	buffer.writeu8(lookupValueToCharacter, value, character)
	buffer.writeu8(lookupCharacterToValue, character, value)
end

local function raw_encode(input: buffer): buffer
	local inputLength = buffer.len(input)
	local inputChunks = math.ceil(inputLength / 3)

	local outputLength = inputChunks * 4
	local output = buffer.create(outputLength)

	for chunkIndex = 1, inputChunks - 1 do
		local inputIndex = (chunkIndex - 1) * 3
		local outputIndex = (chunkIndex - 1) * 4

		local chunk = bit32.byteswap(buffer.readu32(input, inputIndex))

		local value1 = bit32.rshift(chunk, 26)
		local value2 = bit32.band(bit32.rshift(chunk, 20), 0b111111)
		local value3 = bit32.band(bit32.rshift(chunk, 14), 0b111111)
		local value4 = bit32.band(bit32.rshift(chunk, 8), 0b111111)

		buffer.writeu8(output, outputIndex, buffer.readu8(lookupValueToCharacter, value1))
		buffer.writeu8(output, outputIndex + 1, buffer.readu8(lookupValueToCharacter, value2))
		buffer.writeu8(output, outputIndex + 2, buffer.readu8(lookupValueToCharacter, value3))
		buffer.writeu8(output, outputIndex + 3, buffer.readu8(lookupValueToCharacter, value4))
	end

	local inputRemainder = inputLength % 3

	if inputRemainder == 1 then
		local chunk = buffer.readu8(input, inputLength - 1)

		local value1 = bit32.rshift(chunk, 2)
		local value2 = bit32.band(bit32.lshift(chunk, 4), 0b111111)

		buffer.writeu8(output, outputLength - 4, buffer.readu8(lookupValueToCharacter, value1))
		buffer.writeu8(output, outputLength - 3, buffer.readu8(lookupValueToCharacter, value2))
		buffer.writeu8(output, outputLength - 2, padding)
		buffer.writeu8(output, outputLength - 1, padding)
	elseif inputRemainder == 2 then
		local chunk = bit32.bor(
			bit32.lshift(buffer.readu8(input, inputLength - 2), 8),
			buffer.readu8(input, inputLength - 1)
		)

		local value1 = bit32.rshift(chunk, 10)
		local value2 = bit32.band(bit32.rshift(chunk, 4), 0b111111)
		local value3 = bit32.band(bit32.lshift(chunk, 2), 0b111111)

		buffer.writeu8(output, outputLength - 4, buffer.readu8(lookupValueToCharacter, value1))
		buffer.writeu8(output, outputLength - 3, buffer.readu8(lookupValueToCharacter, value2))
		buffer.writeu8(output, outputLength - 2, buffer.readu8(lookupValueToCharacter, value3))
		buffer.writeu8(output, outputLength - 1, padding)
	elseif inputRemainder == 0 and inputLength ~= 0 then
		local chunk = bit32.bor(
			bit32.lshift(buffer.readu8(input, inputLength - 3), 16),
			bit32.lshift(buffer.readu8(input, inputLength - 2), 8),
			buffer.readu8(input, inputLength - 1)
		)

		local value1 = bit32.rshift(chunk, 18)
		local value2 = bit32.band(bit32.rshift(chunk, 12), 0b111111)
		local value3 = bit32.band(bit32.rshift(chunk, 6), 0b111111)
		local value4 = bit32.band(chunk, 0b111111)

		buffer.writeu8(output, outputLength - 4, buffer.readu8(lookupValueToCharacter, value1))
		buffer.writeu8(output, outputLength - 3, buffer.readu8(lookupValueToCharacter, value2))
		buffer.writeu8(output, outputLength - 2, buffer.readu8(lookupValueToCharacter, value3))
		buffer.writeu8(output, outputLength - 1, buffer.readu8(lookupValueToCharacter, value4))
	end

	return output
end

local function raw_decode(input: buffer): buffer
	local inputLength = buffer.len(input)
	local inputChunks = math.ceil(inputLength / 4)

	local inputPadding = 0
	if inputLength ~= 0 then
		if buffer.readu8(input, inputLength - 1) == padding then inputPadding += 1 end
		if buffer.readu8(input, inputLength - 2) == padding then inputPadding += 1 end
	end

	local outputLength = inputChunks * 3 - inputPadding
	local output = buffer.create(outputLength)

	for chunkIndex = 1, inputChunks - 1 do
		local inputIndex = (chunkIndex - 1) * 4
		local outputIndex = (chunkIndex - 1) * 3

		local value1 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, inputIndex))
		local value2 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, inputIndex + 1))
		local value3 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, inputIndex + 2))
		local value4 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, inputIndex + 3))

		local chunk = bit32.bor(
			bit32.lshift(value1, 18),
			bit32.lshift(value2, 12),
			bit32.lshift(value3, 6),
			value4
		)

		local character1 = bit32.rshift(chunk, 16)
		local character2 = bit32.band(bit32.rshift(chunk, 8), 0b11111111)
		local character3 = bit32.band(chunk, 0b11111111)

		buffer.writeu8(output, outputIndex, character1)
		buffer.writeu8(output, outputIndex + 1, character2)
		buffer.writeu8(output, outputIndex + 2, character3)
	end

	if inputLength ~= 0 then
		local lastInputIndex = (inputChunks - 1) * 4
		local lastOutputIndex = (inputChunks - 1) * 3

		local lastValue1 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, lastInputIndex))
		local lastValue2 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, lastInputIndex + 1))
		local lastValue3 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, lastInputIndex + 2))
		local lastValue4 = buffer.readu8(lookupCharacterToValue, buffer.readu8(input, lastInputIndex + 3))

		local lastChunk = bit32.bor(
			bit32.lshift(lastValue1, 18),
			bit32.lshift(lastValue2, 12),
			bit32.lshift(lastValue3, 6),
			lastValue4
		)

		if inputPadding <= 2 then
			local lastCharacter1 = bit32.rshift(lastChunk, 16)
			buffer.writeu8(output, lastOutputIndex, lastCharacter1)

			if inputPadding <= 1 then
				local lastCharacter2 = bit32.band(bit32.rshift(lastChunk, 8), 0b11111111)
				buffer.writeu8(output, lastOutputIndex + 1, lastCharacter2)

				if inputPadding == 0 then
					local lastCharacter3 = bit32.band(lastChunk, 0b11111111)
					buffer.writeu8(output, lastOutputIndex + 2, lastCharacter3)
				end
			end
		end
	end

	return output
end

env.base64encode = function(input)
	return buffer.tostring(raw_encode(buffer.fromstring(input)))
end
env.base64_encode = env.base64encode

env.base64decode = function(encoded)
	return buffer.tostring(raw_decode(buffer.fromstring(encoded)))
end
env.base64_decode = env.base64decode

local base64 = {}

base64.encode = env.base64encode
base64.decode = env.base64decode

env.base64 = base64

env.islclosure = function(func)
	assert(type(func) == "function", "invalid argument #1 to 'islclosure' (function expected, got " .. type(func) .. ") ", 2)
	return debug.info(func, "s") ~= "[C]"
end
env.isluaclosure = env.islclosure

env.iscclosure = function(func)
	assert(type(func) == "function", "invalid argument #1 to 'iscclosure' (function expected, got " .. type(func) .. ") ", 2)
	return debug.info(func, "s") == "[C]"
end

env.newlclosure = function(func)
	assert(type(func) == "function", "invalid argument #1 to 'newlclosure' (function expected, got " .. type(func) .. ") ", 2)
	local cloned = function(...)
		return func(...)
	end
	return cloned
end

env.newcclosure = function(func)
	assert(type(func) == "function", "invalid argument #1 to 'newcclosure' (function expected, got " .. type(func) .. ") ", 2)
	local cloned = coroutine.wrap(function(...)
		while true do
			coroutine.yield(func(...))
		end
	end)
	return cloned
end

env.clonefunction = function(func)
	assert(type(func) == "function", "invalid argument #1 to 'clonefunction' (function expected, got " .. type(func) .. ") ", 2)
	if env.iscclosure(func) then
		return env.newcclosure(func)
	else
		return env.newlclosure(func)
	end
end

local user_agent = "ExternalExecutor"
function env.HttpGet(url, returnRaw)
	assert(type(url) == "string", "invalid argument #1 to 'HttpGet' (string expected, got " .. type(url) .. ") ", 2)
	local returnRaw = returnRaw or true

	local result = env.request({
		Url = url,
		Method = "GET",
		Headers = {
			["User-Agent"] = user_agent
		}
	})

	if returnRaw then
		return result.Body
	end

	return hs:JSONDecode(result.Body)
end
function env.HttpPost(url, body, contentType)
	assert(type(url) == "string", "invalid argument #1 to 'HttpPost' (string expected, got " .. type(url) .. ") ", 2)
	contentType = contentType or "application/json"
	return env.request({
		Url = url,
		Method = "POST",
		body = body,
		Headers = {
			["Content-Type"] = contentType
		}
	})
end
function env.GetObjects(asset)
	return {
		is:LoadLocalAsset(asset)
	}
end

local function GenerateError(object)
	local _, err = xpcall(function()
		object:__namecall()
	end, function()
		return debug.info(2, "f")
	end)
	return err
end

local FirstTest = GenerateError(OverlapParams.new())
local SecondTest = GenerateError(Color3.new())

local cachedmethods = {}
env.getnamecallmethod = function()
	local _, err = pcall(FirstTest)
	local method = if type(err) == "string" then err:match("^(.+) is not a valid member of %w+$") else nil
	if not method then
		_, err = pcall(SecondTest)
		method = if type(err) == "string" then err:match("^(.+) is not a valid member of %w+$") else nil
	end
	local fixerdata = newproxy(true)
	local fixermeta = getmetatable(fixerdata)
	fixermeta.__namecall = function()
		local _, err = pcall(FirstTest)
		local method = if type(err) == "string" then err:match("^(.+) is not a valid member of %w+$") else nil
		if not method then
			_, err = pcall(SecondTest)
			method = if type(err) == "string" then err:match("^(.+) is not a valid member of %w+$") else nil
		end
	end
	fixerdata:__namecall()
	if not method or method == "__namecall" then
		if cachedmethods[coroutine.running()] then
			return cachedmethods[coroutine.running()]
		end
		return nil
	end
	cachedmethods[coroutine.running()] = method
	return method
end

local proxyobject
local proxied = {}
local objects = {}

function ToProxy(...)
	local packed = table.pack(...)
	local function LookTable(t)
		for i, obj in ipairs(t) do
			if rtypeof(obj) == "Instance" then
				if objects[obj] then
					t[i] = objects[obj].proxy
				else
					t[i] = proxyobject(obj)
				end
			elseif typeof(obj) == "table" then
				LookTable(obj)
			else
				t[i] = obj
			end
		end
	end
	LookTable(packed)
	return table.unpack(packed, 1, packed.n)
end

function ToObject(...)
	local packed = table.pack(...)
	local function LookTable(t)
		for i, obj in ipairs(t) do
			if rtypeof(obj) == "userdata" then
				if proxied[obj] then
					t[i] = proxied[obj].object
				else
					t[i] = obj
				end
			elseif typeof(obj) == "table" then
				LookTable(obj)
			else
				t[i] = obj
			end
		end
	end
	LookTable(packed)
	return table.unpack(packed, 1, packed.n)
end

local function index(t, n)
	local data = proxied[t]
	local namecalls = data.namecalls
	local obj = data.object
	if namecalls[n] then
		return function(self, ...)
			return ToProxy(namecalls[n](...))
		end
	end
	local v = obj[n]
	if typeof(v) == "function" then
		return function(self, ...)
			return ToProxy(v(ToObject(self, ...)))
		end
	else
		return ToProxy(v)
	end
end

local function namecall(t, ...)
	local data = proxied[t]
	local namecalls = data.namecalls
	local obj = data.object
	local method = env.getnamecallmethod()
	if namecalls[method] then
		return ToProxy(namecalls[method](...))
	end
	return ToProxy(obj[method](ToObject(t, ...)))
end

local function newindex(t, n, v)
	local data = proxied[t]
	local obj = data.object
	local val = table.pack(ToObject(v))
	obj[n] = table.unpack(val)
end

local function ptostring(t)
	return t.Name
end

function proxyobject(obj, namecalls)
	if objects[obj] then
		return objects[obj].proxy
	end
	namecalls = namecalls or {}
	local proxy = newproxy(true)
	local meta = getmetatable(proxy)
	meta.__index = function(...)return index(...)end
	meta.__namecall = function(...)return namecall(...)end
	meta.__newindex = function(...)return newindex(...)end
	meta.__tostring = function(...)return ptostring(...)end
	meta.__metatable = getmetatable(obj)

	local data = {}
	data.object = obj
	data.proxy = proxy
	data.meta = meta
	data.namecalls = namecalls

	proxied[proxy] = data
	objects[obj] = data
	return proxy
end

local pgame = proxyobject(game, {
	HttpGet = env.HttpGet,
	HttpGetAsync = env.HttpGet,
	HttpPost = env.HttpPost,
	HttpPostAsync = env.HttpPost,
	GetObjects = env.GetObjects
})
env.game = pgame
env.Game = pgame

local pworkspace = proxyobject(workspace)
env.workspace = pworkspace
env.Workspace = pworkspace

local pscript = proxyobject(script)
env.script = pscript

local hui = proxyobject(Instance.new("ScreenGui", cg))
hui.Name = "hidden_ui_container"

for i, v in ipairs(game:GetDescendants()) do
	proxyobject(v)
end
game.DescendantAdded:Connect(proxyobject)

local rInstance = Instance
local fInstance = {}
fInstance.new = function(name, par)
	return proxyobject(rInstance.new(name, ToObject(par)))
end
fInstance.fromExisting = function(obj)
	return proxyobject(rInstance.fromExisting(obj))
end
env.Instance = fInstance

env.getinstances = function()
	local Instances = {}
	for i, v in pairs(objects) do
		table.insert(Instances, v.proxy)
	end
	return Instances
end

env.getnilinstances = function()
	local NilInstances = {}
	for i, v in pairs(objects) do
		if v.proxy.Parent == nil then
			table.insert(NilInstances, v.proxy)
		end
	end
	return NilInstances
end

env.getloadedmodules = function()
	local LoadedModules = {}
	for i, v in pairs(objects) do
		if v.proxy:IsA("ModuleScript") then
			table.insert(LoadedModules, v.proxy)
		end
	end
	return LoadedModules
end

env.getrunningscripts = function()
	local RunningScripts = {}
	for i, v in pairs(objects) do
		if v.proxy:IsA("ModuleScript") then
			table.insert(RunningScripts, v.proxy)
		end
	end
	return RunningScripts
end

env.getscripts = function()
	local Scripts = {}
	for i, v in pairs(objects) do
		if v.proxy:IsA("LocalScript") or v.proxy:IsA("ModuleScript") or v.proxy:IsA("Script") then
			table.insert(Scripts, v.proxy)
		end
	end
	return Scripts
end

env.typeof = function(obj)
	local typ = rtypeof(obj)
	if typ == "userdata" then
		if proxied[obj] then
			return "Instance"
		else
			return typ
		end
	else
		return typ
	end
end

env.gethui = function()
	return hui
end

local crypt = {}

crypt.base64encode = env.base64encode
crypt.base64_encode = env.base64encode
crypt.base64decode = env.base64decode
crypt.base64_decode = env.base64decode
crypt.base64 = base64

crypt.generatekey = function(len)
	local key = ''
	local x = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
	for i = 1, len or 32 do local n = math.random(1, #x) key = key .. x:sub(n, n) end
	return base64.encode(key)
end

crypt.encrypt = function(a, b)
	local result = {}
	a = tostring(a) b = tostring(b)
	for i = 1, #a do
		local byte = string.byte(a, i)
		local keyByte = string.byte(b, (i - 1) % #b + 1)
		table.insert(result, string.char(bit32.bxor(byte, keyByte)))
	end
	return table.concat(result), b
end

crypt.generatebytes = function(len)
	return crypt.generatekey(len)
end

crypt.random = function(len)
	return crypt.generatekey(len)
end

crypt.decrypt = crypt.encrypt

local HashRes = env.request({
	Url = "https://raw.githubusercontent.com/ChimeraLle-Real/Fynex/refs/heads/main/hash",
	Method = "GET"
})
local HashLib = {}

if HashRes and HashRes.Body then
	local func, err = env.loadstring(HashRes.Body)
	if func then
		HashLib = func()
	else
		warn("HasbLib Failed To Load Error: " .. tostring(err))
	end
end

local DrawingRes = env.request({
	Url = "https://raw.githubusercontent.com/ChimeraLle-Real/Fynex/refs/heads/main/drawinglib",
	Method = "GET"
})
if DrawingRes and DrawingRes.Body then
	local func, err = env.loadstring(DrawingRes.Body)
	if func then
		local drawing = func()
		env.Drawing = drawing.Drawing
		for i, v in drawing.functions do
			env[i] = v
		end
	else
		warn("DrawingLib Failed To Load Error: " .. tostring(err))
	end
end

crypt.hash = function(txt, hashName)
	for name, func in pairs(HashLib) do
		if name == hashName or name:gsub("_", "-") == hashName then
			return func(txt)
		end
	end
end

env.crypt = crypt

-- listener

nukedata("", "listen")
task.spawn(function()
	while true do
		local res = nukedata("", "listen")
		if typeof(res) == "table" then
			ExternalExecutor:Destroy()
			break
		end
		if res and #res > 1 then
			task.spawn(function()
				local func, funcerr = env.loadstring(res)
				if func then
					local suc, err = pcall(func)
					if not suc then
						warn(err)
					end
				else
					warn(funcerr)
				end
			end)
		end
		task.wait()
	end
end)

print("Injected!")

return {HideTemp = function() end,GetIsModal = function() end}