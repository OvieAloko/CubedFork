@echo off

call glslangValidator -V -o bin/basic.vert.spirv basic.vert.glsl
call glslangValidator -V -o bin/basic.frag.spirv basic.frag.glsl
pause