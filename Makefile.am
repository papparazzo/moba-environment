## Process this file with automake to generate Makefile.in
SUBDIRS = \
    modules/lib-msghandling/src/moba \
    src

# see https://www.freedesktop.org/software/systemd/man/daemon.html for details
#
AM_DISTCHECK_CONFIGURE_FLAGS = \
  --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

if HAVE_SYSTEMD
systemdsystemunit_DATA = \
  config/moba-environment.service
endif

moba_environmentconfdir=$(sysconfdir)
moba_environmentconf_DATA = \
  config/moba-environment.conf

CLEANFILES = *~
