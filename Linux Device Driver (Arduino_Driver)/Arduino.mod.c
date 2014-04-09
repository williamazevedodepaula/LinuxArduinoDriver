#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x80567cab, "module_layout" },
	{ 0x43eba1e8, "usb_deregister" },
	{ 0xd28de0e8, "usb_register_driver" },
	{ 0x37a0cba, "kfree" },
	{ 0x5bf65cec, "usb_put_dev" },
	{ 0x1158eb04, "usb_register_dev" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x1c7ab1d1, "__mutex_init" },
	{ 0x3db697ee, "usb_get_dev" },
	{ 0xc27eb257, "mutex_unlock" },
	{ 0xf6259959, "usb_deregister_dev" },
	{ 0x84a896a2, "mutex_lock" },
	{ 0x174dd5c1, "dev_set_drvdata" },
	{ 0x3ce9820b, "dev_get_drvdata" },
	{ 0x522c674a, "usb_find_interface" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x6e712077, "kmem_cache_alloc_trace" },
	{ 0xaa5b0f7, "kmalloc_caches" },
	{ 0xecb9e7f, "usb_bulk_msg" },
	{ 0x20000329, "simple_strtoul" },
	{ 0x50eedeb8, "printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "E37F556E2A39C32EF706CED");
