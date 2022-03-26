Import("configure.lua")

--- Setup Config -------
config = NewConfig()
config:Add(OptCCompiler("compiler"))
config:Add(OptTestCompileC("stackprotector", "int main(){return 0;}", "-fstack-protector -fstack-protector-all"))
config:Add(OptTestCompileC("minmacosxsdk", "int main(){return 0;}", "-mmacosx-version-min=10.5 -isysroot /Developer/SDKs/MacOSX10.5.sdk"))
config:Add(OptTestCompileC("macosxppc", "int main(){return 0;}", "-arch ppc"))
config:Add(OptLibrary("zlib", "zlib.h", false))
config:Finalize("config.lua")

-- data compiler
function Script(name)
	if family == "windows" then
		return str_replace(name, "/", "\\")
	end
	return "python " .. name
end

function CHash(output, ...)
	local inputs = TableFlatten({...})

	output = Path(output)

	-- compile all the files
	local cmd = Script("scripts/cmd5.py") .. " "
	for index, inname in ipairs(inputs) do
		cmd = cmd .. Path(inname) .. " "
	end

	cmd = cmd .. " > " .. output

	AddJob(output, "cmd5 " .. output, cmd)
	for index, inname in ipairs(inputs) do
		AddDependency(output, inname)
	end
	AddDependency(output, "scripts/cmd5.py")
	return output
end

--[[
function DuplicateDirectoryStructure(orgpath, srcpath, dstpath)
	for _,v in pairs(CollectDirs(srcpath .. "/")) do
		MakeDirectory(dstpath .. "/" .. string.sub(v, string.len(orgpath)+2))
		DuplicateDirectoryStructure(orgpath, v, dstpath)
	end
end

DuplicateDirectoryStructure("src", "src", "objs")
]]

function ResCompile(scriptfile)
	scriptfile = Path(scriptfile)
	if config.compiler.driver == "cl" then
		output = PathBase(scriptfile) .. ".res"
		AddJob(output, "rc " .. scriptfile, "rc /fo " .. output .. " " .. scriptfile)
	elseif config.compiler.driver == "gcc" then
		output = PathBase(scriptfile) .. ".coff"
		AddJob(output, "windres " .. scriptfile, "windres -i " .. scriptfile .. " -o " .. output)
	end
	AddDependency(output, scriptfile)
	return output
end

function Dat2c(datafile, sourcefile, arrayname)
	datafile = Path(datafile)
	sourcefile = Path(sourcefile)

	AddJob(
		sourcefile,
		"dat2c " .. PathFilename(sourcefile) .. " = " .. PathFilename(datafile),
		Script("scripts/dat2c.py").. "\" " .. sourcefile .. " " .. datafile .. " " .. arrayname
	)
	AddDependency(sourcefile, datafile)
	return sourcefile
end

function ContentCompile(action, output)
	output = Path(output)
	AddJob(
		output,
		action .. " > " .. output,
		--Script("datasrc/compile.py") .. "\" ".. Path(output) .. " " .. action
		Script("datasrc/compile.py") .. " " .. action .. " > " .. Path(output)
	)
	AddDependency(output, Path("datasrc/content.py")) -- do this more proper
	AddDependency(output, Path("datasrc/network.py"))
	AddDependency(output, Path("datasrc/compile.py"))
	AddDependency(output, Path("datasrc/datatypes.py"))
	return output
end

-- Content Compile
network_source = ContentCompile("network_source", "src/game/generated/protocol.cpp")
network_header = ContentCompile("network_header", "src/game/generated/protocol.h")
server_content_source = ContentCompile("server_content_source", "src/game/generated/server_data.cpp")
server_content_header = ContentCompile("server_content_header", "src/game/generated/server_data.h")

AddDependency(network_source, network_header)
AddDependency(server_content_source, server_content_header)

nethash = CHash("src/game/generated/nethash.cpp", "src/engine/shared/protocol.h", "src/game/generated/protocol.h", "src/game/tuning.h", "src/game/gamecore.cpp", network_header)

icu_depends = {}
server_link_other = {}

if family == "windows" then
	if platform == "win32" then
		-- Add ICU because its a HAVE to
		if config.compiler.driver == "cl" then
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icudt53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icuin53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icuuc53.dll"))
		elseif config.compiler.driver == "gcc" then
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icudt53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icuin53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icuuc53.dll"))
		end
	else
		-- Add ICU because its a HAVE to
		if config.compiler.driver == "cl" then
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icudt53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icuin53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icuuc53.dll"))
		elseif config.compiler.driver == "gcc" then
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icudt53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icuin53.dll"))
			table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icuuc53.dll"))
		end
	end
	table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/mysqlcppconn.dll"))
	table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/libmysql.dll"))

	if config.compiler.driver == "cl" then
		server_link_other = {ResCompile("other/icons/teeworlds_srv_cl.rc")}
	elseif config.compiler.driver == "gcc" then
		server_link_other = {ResCompile("other/icons/teeworlds_srv_gcc.rc")}
	end
end

function Intermediate_Output(settings, input)
	return "objs/" .. string.sub(PathBase(input), string.len("src/")+1) .. settings.config_ext
end

function build(settings)
	-- apply compiler settings
	config.compiler:Apply(settings)
	
	--settings.objdir = Path("objs")
	settings.cc.Output = Intermediate_Output

	if config.compiler.driver == "cl" then
		settings.cc.flags:Add("/wd4244", "/wd4577")
	else
		settings.cc.flags:Add("-Wall", "-fno-exceptions")
		if family == "windows" then
			-- disable visibility attribute support for gcc on windows
			settings.cc.defines:Add("NO_VIZ")
		elseif platform == "macosx" then
			settings.cc.flags:Add("-mmacosx-version-min=10.5")
			settings.link.flags:Add("-mmacosx-version-min=10.5")
			if config.minmacosxsdk.value == 1 then
				settings.cc.flags:Add("-isysroot /Developer/SDKs/MacOSX10.5.sdk")
				settings.link.flags:Add("-isysroot /Developer/SDKs/MacOSX10.5.sdk")
			end
		elseif config.stackprotector.value == 1 then
			settings.cc.flags:Add("-fstack-protector", "-fstack-protector-all")
			settings.link.flags:Add("-fstack-protector", "-fstack-protector-all")
		end
	end

	-- set some platform specific settings
	settings.cc.includes:Add("src")

	if family == "unix" then
		if platform == "macosx" then
			settings.cc.flags_cxx:Add("-stdlib=libc++")
			settings.cc.includes:Add("/usr/local/opt/icu4c/include")
			settings.link.libs:Add("icui18n")
			settings.link.libs:Add("icuuc")
			settings.link.libs:Add("c++")
			settings.link.libpath:Add("/usr/local/opt/icu4c/lib")
			settings.link.frameworks:Add("Carbon")
			settings.link.frameworks:Add("AppKit")
		else
			settings.link.libs:Add("pthread")
			-- add ICU for linux
			if ExecuteSilent("pkg-config icu-uc icu-i18n") == 0 then
			end

			settings.cc.flags:Add("`pkg-config --cflags icu-uc icu-i18n`")
			settings.link.flags:Add("`pkg-config --libs icu-uc icu-i18n`")
		end
		
		if platform == "solaris" then
		    settings.link.flags:Add("-lsocket")
		    settings.link.flags:Add("-lnsl")
		end
	elseif family == "windows" then
		settings.link.libs:Add("gdi32")
		settings.link.libs:Add("user32")
		settings.link.libs:Add("ws2_32")
		settings.link.libs:Add("ole32")
		settings.link.libs:Add("shell32")
		settings.link.libs:Add("advapi32")

		-- add ICU also here
		settings.cc.includes:Add("other\\icu\\include")
	end

	-- compile zlib if needed
	if config.zlib.value == 1 then
		settings.link.libs:Add("z")
		if config.zlib.include_path then
			settings.cc.includes:Add(config.zlib.include_path)
		end
		zlib = {}
	else
		zlib = Compile(settings, Collect("src/engine/external/zlib/*.c"))
		settings.cc.includes:Add("src/engine/external/zlib")
	end

	-- build the small libraries
	json = Compile(settings, "src/engine/external/json-parser/json.c")
	md5 = Compile(settings, Collect("src/engine/external/md5/*.c"))

	-- build game components
	engine_settings = settings:Copy()
	server_settings = engine_settings:Copy()
	launcher_settings = engine_settings:Copy()

	if family == "unix" then
		if platform == "macosx" then
			launcher_settings.link.frameworks:Add("Cocoa")
		end

	elseif family == "windows" then
		-- Add ICU because its a HAVE to
		if platform == "win32" then
			if config.compiler.driver == "cl" then
				server_settings.link.libpath:Add("other/icu/vc/lib32")
			elseif config.compiler.driver == "gcc" then
				server_settings.link.libpath:Add("other/icu/gcc/lib32")
			end
			server_settings.link.libs:Add("icudt")
			server_settings.link.libs:Add("icuin")
			server_settings.link.libs:Add("icuuc")
		else
			if config.compiler.driver == "cl" then
				server_settings.link.libpath:Add("other/icu/vc/lib64")
			elseif config.compiler.driver == "gcc" then
				server_settings.link.libpath:Add("other/icu/gcc/lib64")
			end
			server_settings.link.libs:Add("icudt")
			server_settings.link.libs:Add("icuin")
			server_settings.link.libs:Add("icuuc")
		end
	end
	
	engine = Compile(engine_settings, Collect("src/engine/shared/*.cpp", "src/base/*.c"))
	server = Compile(server_settings, Collect("src/engine/server/*.cpp"))
	teeuniverses = Compile(server_settings, Collect("src/teeuniverses/*.cpp", "src/teeuniverses/components/*.cpp", "src/teeuniverses/system/*.cpp"))

	game_shared = Compile(settings, Collect("src/game/*.cpp"), nethash, network_source)
	game_server = Compile(settings, CollectRecursive("src/game/server/*.cpp"), server_content_source)

	server_osxlaunch = {}
	if platform == "macosx" then
		server_osxlaunch = Compile(launcher_settings, "src/osxlaunch/server.m")
	end

	-- build server
	server_exe = Link(server_settings, "teeworlds_srv", engine, server,
		game_shared, game_server, zlib, md5, server_link_other, json, teeuniverses)

	serverlaunch = {}
	if platform == "macosx" then
		serverlaunch = Link(launcher_settings, "serverlaunch", server_osxlaunch)
	end

	-- make targets
	s = PseudoTarget("server".."_"..settings.config_name, server_exe, serverlaunch, icu_depends)

	all = PseudoTarget(settings.config_name, c, s, v, m, t)
	return all
end


debug_settings = NewSettings()
debug_settings.config_name = "debug"
debug_settings.config_ext = "_d"
debug_settings.debug = 1
debug_settings.optimize = 0
debug_settings.cc.defines:Add("CONF_DEBUG")

release_settings = NewSettings()
release_settings.config_name = "release"
release_settings.config_ext = ""
release_settings.debug = 0
release_settings.optimize = 1
release_settings.cc.defines:Add("CONF_RELEASE")

if platform == "macosx" then
	debug_settings_ppc = debug_settings:Copy()
	debug_settings_ppc.config_name = "debug_ppc"
	debug_settings_ppc.config_ext = "_ppc_d"
	debug_settings_ppc.cc.flags:Add("-arch ppc")
	debug_settings_ppc.link.flags:Add("-arch ppc")
	debug_settings_ppc.cc.defines:Add("CONF_DEBUG")

	release_settings_ppc = release_settings:Copy()
	release_settings_ppc.config_name = "release_ppc"
	release_settings_ppc.config_ext = "_ppc"
	release_settings_ppc.cc.flags:Add("-arch ppc")
	release_settings_ppc.link.flags:Add("-arch ppc")
	release_settings_ppc.cc.defines:Add("CONF_RELEASE")

	ppc_d = build(debug_settings_ppc)
	ppc_r = build(release_settings_ppc)

	if arch == "ia32" or arch == "amd64" then
		debug_settings_x86 = debug_settings:Copy()
		debug_settings_x86.config_name = "debug_x86"
		debug_settings_x86.config_ext = "_x86_d"
		debug_settings_x86.cc.flags:Add("-arch i386")
		debug_settings_x86.link.flags:Add("-arch i386")
		debug_settings_x86.cc.defines:Add("CONF_DEBUG")

		release_settings_x86 = release_settings:Copy()
		release_settings_x86.config_name = "release_x86"
		release_settings_x86.config_ext = "_x86"
		release_settings_x86.cc.flags:Add("-arch i386")
		release_settings_x86.link.flags:Add("-arch i386")
		release_settings_x86.cc.defines:Add("CONF_RELEASE")
	
		x86_d = build(debug_settings_x86)
		x86_r = build(release_settings_x86)
	end

	if arch == "amd64" then
		debug_settings_x86_64 = debug_settings:Copy()
		debug_settings_x86_64.config_name = "debug_x86_64"
		debug_settings_x86_64.config_ext = "_x86_64_d"
		debug_settings_x86_64.cc.flags:Add("-arch x86_64")
		debug_settings_x86_64.link.flags:Add("-arch x86_64")
		debug_settings_x86_64.cc.defines:Add("CONF_DEBUG")

		release_settings_x86_64 = release_settings:Copy()
		release_settings_x86_64.config_name = "release_x86_64"
		release_settings_x86_64.config_ext = "_x86_64"
		release_settings_x86_64.cc.flags:Add("-arch x86_64")
		release_settings_x86_64.link.flags:Add("-arch x86_64")
		release_settings_x86_64.cc.defines:Add("CONF_RELEASE")

		x86_64_d = build(debug_settings_x86_64)
		x86_64_r = build(release_settings_x86_64)
	end

	DefaultTarget("game_debug_x86")
	
	if config.macosxppc.value == 1 then
		if arch == "ia32" then
			PseudoTarget("release", ppc_r, x86_r)
			PseudoTarget("debug", ppc_d, x86_d)
			PseudoTarget("server_release", "server_release_ppc", "server_release_x86")
			PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86")
		elseif arch == "amd64" then
			PseudoTarget("release", ppc_r, x86_r, x86_64_r)
			PseudoTarget("debug", ppc_d, x86_d, x86_64_d)
			PseudoTarget("server_release", "server_release_ppc", "server_release_x86", "server_release_x86_64")
			PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86", "server_debug_x86_64")
		else
			PseudoTarget("release", ppc_r)
			PseudoTarget("debug", ppc_d)
			PseudoTarget("server_release", "server_release_ppc")
			PseudoTarget("server_debug", "server_debug_ppc")
		end
	else
		if arch == "ia32" then
			PseudoTarget("release", x86_r)
			PseudoTarget("debug", x86_d)
			PseudoTarget("server_release", "server_release_x86")
			PseudoTarget("server_debug", "server_debug_x86")
		elseif arch == "amd64" then
			PseudoTarget("release", x86_r, x86_64_r)
			PseudoTarget("debug", x86_d, x86_64_d)
			PseudoTarget("server_release", "server_release_x86", "server_release_x86_64")
			PseudoTarget("server_debug", "server_debug_x86", "server_debug_x86_64")
		end
	end
else
	build(debug_settings)
	build(release_settings)
	DefaultTarget("server_debug")
end
