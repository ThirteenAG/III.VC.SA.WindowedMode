workspace "III.VC.SA.WindowedMode"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x32"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
      
project "III.VC.SA.WindowedMode"
   kind "SharedLib"
   language "C++"
   targetdir "data/%{cfg.buildcfg}"
   targetextension ".asi"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/ThirteenAG\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.WindowedMode\"" }
   
   files { "source/cpp/*.h" }
   files { "source/cpp/*.cpp", "source/*.c" }
   files { "source/cpp/*.rc" }

   includedirs { "source/cpp" }
   includedirs { "source/cpp/d3d8" }
   includedirs { "external/injector/include" }
   includedirs { "external/IniReader" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
	  characterset ("MBCS")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  characterset ("MBCS")
	  targetdir "data/"
	  
	  
project "III.VC.SA.CoordsManager"
   kind "WindowedApp"
   language "C#"
   location "source/cs"
   targetdir "data/%{cfg.buildcfg}"
   targetextension ".exe"
   icon "source/cs/radar_player_target.ico"
      
   files { "source/cs/*.*" }
   files { "source/cs/Properties/*.*" }
   files { "source/cs/Resources/*.*" }

   links { "System",
		   "System.Core",
		   "System.Xml.Linq",
		   "System.Data.DataSetExtensions",
		   "Microsoft.CSharp",
		   "System.Data",
		   "System.Deployment",
		   "System.Drawing",
		   "System.Net.Http",
		   "System.Windows.Forms",
		   "System.Xml"
	}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
	  characterset ("MBCS")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  characterset ("MBCS")
	  targetdir "data/"
