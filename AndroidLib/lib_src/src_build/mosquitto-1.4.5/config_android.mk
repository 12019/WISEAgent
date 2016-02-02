# =============================================================================
# User configuration section.
#
# These options control compilation on all systems apart from Windows and Mac
# OS X. Use CMake to compile on Windows and Mac.
#
# Largely, these are options that are designed to make mosquitto run more
# easily in restrictive environments by removing features.
#
# Modify the variable below to enable/disable features.
#
# Can also be overriden at the command line, e.g.:
#
# make WITH_TLS=no
# =============================================================================

# Uncomment to compile the broker with tcpd/libwrap support.
#WITH_WRAP:=yes

# Comment out to disable SSL/TLS support in the broker and client.
# Disabling this will also mean that passwords must be stored in plain text. It
# is strongly recommended that you only disable WITH_TLS if you are not using
# password authentication at all.
WITH_TLS:=yes

# Comment out to disable TLS/PSK support in the broker and client. Requires
# WITH_TLS=yes.
# This must be disabled if using openssl < 1.0.
WITH_TLS_PSK:=yes

# Comment out to disable client client threading support.
WITH_THREADING:=yes

# Comment out to remove bridge support from the broker. This allow the broker
# to connect to other brokers and subscribe/publish to topics. You probably
# want to leave this included unless you want to save a very small amount of
# memory size and CPU time.
WITH_BRIDGE:=yes

# Comment out to remove persistent database support from the broker. This
# allows the broker to store retained messages and durable subscriptions to a
# file periodically and on shutdown. This is usually desirable (and is
# suggested by the MQTT spec), but it can be disabled if required.
WITH_PERSISTENCE:=yes

# Comment out to remove memory tracking support from the broker. If disabled,
# mosquitto won't track heap memory usage nor export '$SYS/broker/heap/current
# size', but will use slightly less memory and CPU time.
WITH_MEMORY_TRACKING:=yes

# Compile with database upgrading support? If disabled, mosquitto won't
# automatically upgrade old database versions.
# Not currently supported.
#WITH_DB_UPGRADE:=yes

# Comment out to remove publishing of the $SYS topic hierarchy containing
# information about the broker state.
WITH_SYS_TREE:=yes

# Build with SRV lookup support.
WITH_SRV:=yes

# Build using libuuid for clientid generation (Linux only - please report if
# supported on your platform).
WITH_UUID:=yes

# Build with websockets support on the broker.
WITH_WEBSOCKETS:=no

# Use elliptic keys in broker
WITH_EC:=yes

# Build man page documentation by default.
WITH_DOCS:=yes

# Build with client support for SOCK5 proxy.
WITH_SOCKS:=yes

# =============================================================================
# End of user configuration
# =============================================================================

VERSION=1.4.5

LIB_CFLAGS:=-fPIC
CLIENT_CFLAGS:=-DVERSION="\"${VERSION}\""

ifeq ($(WITH_TLS),yes)
#	LIB_CFLAGS:=$(LIB_CFLAGS) -DWITH_TLS
	CLIENT_CFLAGS:=$(CLIENT_CFLAGS) -DWITH_TLS

	ifeq ($(WITH_TLS_PSK),yes)
#		LIB_CFLAGS:=$(LIB_CFLAGS) -DWITH_TLS_PSK
		CLIENT_CFLAGS:=$(CLIENT_CFLAGS) -DWITH_TLS_PSK
	endif
endif

ifeq ($(WITH_THREADING),yes)
	LIB_CFLAGS:=$(LIB_CFLAGS) -DWITH_THREADING
endif

ifeq ($(WITH_SOCKS),yes)
	CLIENT_CFLAGS:=$(CLIENT_CFLAGS) -DWITH_SOCKS
endif

ifeq ($(WITH_SRV),yes)
#	LIB_CFLAGS:=$(LIB_CFLAGS) -DWITH_SRV
	CLIENT_CFLAGS:=$(CLIENT_CFLAGS) -DWITH_SRV
endif


