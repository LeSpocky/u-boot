#
# (C) Copyright 2003-2008
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2008
# Stelian Pop <stelian.pop@leadtechdesign.com>
# Lead Tech Design <www.leadtechdesign.com>
#
# Copyright 2010,2012 ifak system GmbH
# Copyright 2019 Thorsis Technologies GmbH
#
# SPDX-License-Identifier:	GPL-2.0+
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).o

COBJS-y				+= at91sam9g20ncl.o
COBJS-$(CONFIG_AT91_LED)	+= led.o
COBJS-$(CONFIG_AT91_BUTTON)	+= button.o
COBJS-$(CONFIG_FPGA)		+= fpga.o
COBJS-y				+= backplane.o
COBJS-y				+= hrttest.o
COBJS-$(CONFIG_HAS_DATAFLASH)	+= partition.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
# vim: ft=make noet sts=0 ts=8 sw=8 sr
