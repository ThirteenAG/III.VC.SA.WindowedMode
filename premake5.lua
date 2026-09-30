newoption {
    trigger     = "with-version",
    value       = "STRING",
    description = "Current version",
}


-- The folder a project is deployed to, and the game it is started from when debugging,
-- is the path of one machine and does not belong in the repository. It is read from a
-- `.env` file next to this script, which is not tracked by git and holds one
-- `<KEY>=<folder>` line per game (quotes and a trailing slash are optional). A project
-- whose key is missing is not deployed at all.
local envkeys = nil
function envdir(key)
   if not envkeys then
      envkeys = {}
      local text = io.readfile(path.join(_SCRIPT_DIR, ".env")) or ""
      for line in text:gmatch("[^\r\n]+") do
         local k, v = line:match("^%s*([%w_]+)%s*=%s*(.-)%s*$")
         if k and v ~= "" then
            v = v:gsub('^"', ""):gsub('"$', ""):gsub("^'", ""):gsub("'$", "")
            envkeys[k] = v
         end
      end
   end

   local value = envkeys[key]
   if not value then return nil end

   value = value:gsub("[%s\\/]+$", "")
   if value == "" then return nil end

   return path.translate(value)
end

-- Deploys the built .asi into the script folder of the game that `key` names in the .env
-- file, and starts the game from there when debugging. Only a plugin that is already
-- installed in the game folder is replaced, a folder without one is left alone.
function setpaths(key, exepath, scriptspath)
   scriptspath = scriptspath or "scripts/"
   local gamepath = envdir(key)
   if gamepath then
      local target = gamepath .. "\\" .. path.translate(scriptspath)
      postbuildcommands {
         "if exist \"" .. target .. "$(TargetFileName)\" copy /y \"$(TargetPath)\" \"" .. target .. "\"",
      }
      debugdir (gamepath)
      if (exepath) then
         debugcommand (gamepath .. "\\" .. path.translate(exepath))
      end
   end
end

workspace "III.VC.SA.WindowedMode"
   configurations { "Release", "Gta3", "GtaVC", "GtaSA" }
   platforms { "Win32" }
   architecture "x86"
   characterset ("MBCS")
   staticruntime "on"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   cppdialect "C++latest"
      
project "III.VC.SA.WindowedMode"
   kind "SharedLib"
   language "C++"
   targetdir "data/%{cfg.buildcfg}"
   targetextension ".asi"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   local major = os.date("%d")
   local minor = os.date("%m")
   local build = os.date("%Y")
   local revision = os.date("%H") .. os.date("%M")

   if _OPTIONS["with-version"] then
      local t = {}
      for i in _OPTIONS["with-version"]:gmatch("([^.]+)") do
         t[#t + 1], _ = i:gsub("%D+", "")
      end
      while #t < 4 do t[#t + 1] = 0 end
      major    = math.min(tonumber(t[1]), 255)
      minor    = math.min(tonumber(t[2]), 255)
      build    = math.min(tonumber(t[3]), 65535)
      revision = math.min(tonumber(t[4]), 65535)
   end

   local githash = ""
   local f = io.popen("git rev-parse --short HEAD")
   if f then
      githash = f:read("*a"):gsub("%s+", "")
      f:close()
   end

   local productVersion = major .. "." .. minor .. "." .. build .. "." .. revision
   if githash ~= "" then
      productVersion = productVersion .. "-" .. githash
   end

   defines { "rsc_FileVersion_MAJOR=" .. major }
   defines { "rsc_FileVersion_MINOR=" .. minor }
   defines { "rsc_FileVersion_BUILD=" .. build }
   defines { "rsc_FileVersion_REVISION=" .. revision }
   defines { "rsc_FileVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }
   defines { "rsc_ProductVersion=\"" .. productVersion .. "\"" }
   defines { "rsc_GitSHA1=\"" .. githash .. "\"" }
   defines { "rsc_GitSHA1W=L\"" .. githash .. "\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/ThirteenAG\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.WindowedMode\"" }
   
   files { "source/*.h", "source/*.ixx" }
   files { "source/*.cpp", "source/*.c" }
   files { "source/*.rc", "external/hooking/*.cpp", "external/hooking/*.h" }
   files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
   files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }

   includedirs { "source", "external/hooking" }
   includedirs { "source/d3d8" }
   includedirs { "external/injector/safetyhook/include" }
   includedirs { "external/injector/zydis" }
   includedirs { "external/injector/include" }
   includedirs { "external/IniReader" }

   filter "configurations:Gta3"
      defines { "DEBUG" }
      symbols "on"
      setpaths("GTAIII_DIR", "gta3.exe")
      
   filter "configurations:GtaVC"
      defines { "DEBUG" }
      symbols "on"
      setpaths("GRAND_THEFT_AUTO_VICE_CITY_DIR", "gta-vc.exe")
      
   filter "configurations:GtaSA"
      defines { "DEBUG" }
      symbols "on"
      setpaths("GTA_SAN_ANDREAS_DIR", "gta_sa.exe")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "on"
      symbols "On"
      targetdir "data"
