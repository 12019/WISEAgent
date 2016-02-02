include ../../Makefile.common

#Makefile for Libsmart

LIBRARY_PATH_SMART = smartmontools/
LIBRARY_CHECK_SMART = smartcheck/

default:
	$(MAKE) -C $(LIBRARY_PATH_SMART)
	$(MAKE) -C $(LIBRARY_CHECK_SMART)
	@cp $(LIBRARY_PATH_SMART)/smartctl .
	@cp $(LIBRARY_CHECK_SMART)/*.so .

clean:
	@rm libsmart.so
	@rm smartctl
	$(MAKE) -C $(LIBRARY_PATH_SMART) clean
	$(MAKE) -C $(LIBRARY_CHECK_SMART) clean
	
install: 
	cp -d ./libsmart.so $(INSTALL_OUTPUT_DIR)
	cp -d ./smartctl $(INSTALL_OUTPUT_DIR)