-- premake5.lua
workspace "raytracedemo"
   configurations { "Debug", "Release" }

   project "raytracedemo"
      kind "ConsoleApp"
      language "C++"
      cppdialect "C++20"
      architecture "x86_64"
      targetdir "bin/%{cfg.buildcfg}"
         
      files { 
         "src/**.cpp",
         "src/**.h",
      }

      filter "configurations:Debug"
         defines { "_DEBUG", "DEBUG" }
         symbols "On"

      filter "configurations:Release"
         defines { "NDEBUG" }
         optimize "Full"