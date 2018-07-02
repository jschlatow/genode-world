SRC_DIR = src/proxy/remote_rom/backend/nic_ip/server
include $(GENODE_DIR)/repos/base/recipes/src/content.inc

MIRROR_FROM_OS_DIR := lib/mk/net.mk include/net src/lib/net
MIRROR_FROM_REP_DIR := lib/mk/remote_rom_backend.inc lib/mk/remote_rom_backend_nic_ip.mk include/remote_rom src/lib/remote_rom/backend/nic_ip src/proxy/remote_rom/server src/proxy/remote_rom/backend/nic_ip/target.inc

content: $(MIRROR_FROM_REP_DIR) $(MIRROR_FROM_OS_DIR)

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

$(MIRROR_FROM_OS_DIR):
	mkdir -p $(dir $@); cp -r $(REP_DIR)/../os/$@ $(dir $@)
