#
# \brief  Framebuffer driver specific for zynq (adv7511) systems
# \author ida
# \date   2016
#

TARGET   = fb_drv
REQUIRES = zynq_i2c zynq_vdma
SRC_CC   = main.cc
LIBS     = base stdcxx
INC_DIR += $(PRG_DIR)

vpath main.cc $(PRG_DIR)

