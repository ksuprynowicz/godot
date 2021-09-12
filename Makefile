
.PHONY: default

default: 
	scons p=linuxbsd -j`nproc` target=release_debug use_llvm=yes use_lld=yes module_webm_enabled=no use_static_gcc=yes optimize=size use_thinlto=yes debug_symbols=no