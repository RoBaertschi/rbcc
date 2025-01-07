#!/usr/bin/env lua
-- A simple build script that generates ninja build files

--> Build Configuration --
local files = { "main.c", "lexer.c", "utf8proc.c", "str.c", "ast.c", "parser.c" }
local target = "build/rbc"
local compiler = "clang"
local cflags = "-g -std=c11 -O2 -Wall -Wextra -Wpedantic"
local ldflags = ""
--< Build Configuration --

-- Build Logic

local is_windows = package.config:sub(1, 1) == "\\"

local ninja = io.open("build.ninja", "w+")

if ninja == nil then
	error("could not open build.ninja")
end
local s = "    "

---@param name string
---@param content string
local function var(name, content)
	ninja:write(name .. " = " .. content .. "\n")
end

---@param r string
---@param content {[string]: string}
---@return string
local function rule(r, content)
	ninja:write("rule " .. r .. "\n")
	for key, value in pairs(content) do
		ninja:write(s .. key .. " = " .. value .. "\n")
	end
	return r
end

---@param output string
---@param inputs string[]
---@param r string
local function build(output, inputs, r)
	ninja:write("build " .. output .. ": " .. r)
	for _, input in ipairs(inputs) do
		ninja:write(" " .. input)
	end
	ninja:write("\n")
end

if compiler == "clang" then
	cflags = cflags .. " -Wno-nullability-extension" -- we can ignore the warning about the nullability extension because we only add it for clang.
end

var("ninja_required_version", "1.2")
var("cflags", cflags) -- Flags passed to gcc
var("ldflags", ldflags) -- Flags passed to gcc when linking

---@type { input: string, output: string }[]
local files_to_build = {}
---@type string[]
local object_files = {}

for _, file in ipairs(files) do
	local object_file = "build/" .. file:gsub("%.c", ".o")

	table.insert(files_to_build, { input = file, output = object_file })
	table.insert(object_files, object_file)
end

var("builddir", "build/")

local compdb = rule("compdb", {
	command = "ninja -t compdb > $out",
})
local cc = rule("cc", {
	command = compiler .. " $cflags -c $in -o $out -MD -MF $out.d",
	depfile = "$out.d",
	deps = "gcc",
})
local ld = rule("ld", {
	command = compiler .. " $ldflags $in -o $out",
})

for _, to_build in ipairs(files_to_build) do
	build(to_build.output, { to_build.input }, cc)
end

build(target, object_files, ld)

local function copy_table(t)
	local u = {}
	for k, v in pairs(t) do
		u[k] = v
	end
	return setmetatable(u, getmetatable(t))
end

local all = copy_table(files)
table.insert(all, target)
build("all", all, "phony")

build("compile_commands.json", { "all" }, compdb)

ninja:close()

local _, _, res = os.execute("ninja")
if res ~= 0 then
	os.exit(res)
end
