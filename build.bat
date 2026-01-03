@echo off
cl /LD ^
   /I "C:\Program Files\Python312\include" ^
   smart_dict.c ^
   /link /LIBPATH:"C:\Program Files\Python312\libs" python312.lib ^
   /OUT:smart_dict.pyd
