#
# Makefile for the zeusis leveled module debugger
#

obj-y += hawkeyebmsmode.o
ifdef CONFIG_COMPAT
hawkeyebmsmode-objs := hawkeyebms.o hawkeye_bms_port.o hawkeye_bms_inf.o hawkeye_bms_compat.o
else
hawkeyebmsmode-objs := hawkeyebms.o hawkeye_bms_port.o hawkeye_bms_inf.o
endif