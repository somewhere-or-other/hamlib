# This file is part of MXE.
# See index.html for further information.

PKG             := hamlib
$(PKG)_IGNORE   :=
$(PKG)_CHECKSUM := d07a90687ba314e075b0088faeb1e7e9bb7f6337
$(PKG)_SUBDIR   := hamlib-$($(PKG)_VERSION)
$(PKG)_FILE     := hamlib-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://ignum.dl.sourceforge.net/project/hamlib/hamlib/$($(PKG)_VERSION)/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc libxml2 libtool

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
        --host='$(TARGET)' \
        --with-included-ltdl \
        --prefix='$(PREFIX)/$(TARGET)' \
        --enable-static
    $(MAKE) -C '$(1)' -j '$(JOBS)' install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS=
endef
