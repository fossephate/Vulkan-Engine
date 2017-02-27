rmdir "bin\x64\Debug"
robocopy "build\x64\Debug" "bin\x64\Debug" /E
robocopy "src\assets" "bin\x64\Debug\assets" /E
pause