
.PHONY: bin/libgodot.linuxbsd.opt.64.so

bin/libgodot.linuxbsd.opt.64.so: 
	scons p=linuxbsd library_type=shared_library
	cp -f bin/libgodot.linuxbsd.opt.64.so ${MIX_APP_PATH}/priv/