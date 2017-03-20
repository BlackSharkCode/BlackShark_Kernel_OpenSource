#
# Makefile for the zeusis leveled module debugger
#
obj-y += hawkeyepmsmode.o
ifdef CONFIG_COMPAT
hawkeyepmsmode-objs := hawkeyepms.o hawkeye_pms_compat.o hawkeye_pms_port.o hawkeye_pms_inf.o
else
hawkeyepmsmode-objs := hawkeyepms.o hawkeye_pms_port.o hawkeye_pms_inf.o
endif