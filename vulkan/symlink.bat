


cd C:\Users\Matt\Documents\Visual Studio 2017\Projects\graphicalProjects\vulkan

rem mklink /D ".\assets" "src\assets"

mklink /D "./build\x64\Debug\assets" "..\..\..\src\assets"

mklink /D "./build\x64\Release\assets" "..\..\..\src\assets"

mklink /D "./build\Win32\Debug\assets" "..\..\..\src\assets"

mklink /D "./build\Win32\Release\assets" "..\..\..\src\assets"


pause