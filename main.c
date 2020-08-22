/*
This file is part of Catherine Full Body HD Patch
Copyright © 2020 浅倉麗子

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdbool.h>

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>

#include <fnblit.h>
#include <psp2dbg.h>
#include <taihen.h>

extern char _binary_font_sfn_start[];

#define CFB_MOD_NAME "xrd758_psp2"

#define PATCH_720         0
#define PATCH_544_MSAA_4X 1

#if !(PATCH_MODE == PATCH_720 || PATCH_MODE == PATCH_544_MSAA_4X)
	#pragma GCC error "Invalid PATCH_MODE"
#endif

#define SCALE_X (1280.0 / 960.0)
#define SCALE_Y (720.0 / 540.0)

#define UNUSED __attribute__ ((unused))
#define USED __attribute__ ((used))

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while (0)

#define N_INJECT 11
static SceUID inject_id[N_INJECT];

#define N_HOOK 8
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

static SceUID inject_data(int idx, int mod, int seg, int ofs, void *data, int size) {
	SceUID ret = taiInjectData(mod, seg, ofs, data, size);
	if (ret >= 0) {
		SCE_DBG_LOG_INFO("Injected %d UID %08X\n", idx, ret);
		inject_id[idx] = ret;
	} else {
		SCE_DBG_LOG_ERROR("Failed to inject %d error %08X\n", idx, ret);
	}
	return ret;
}
#define INJECT_DATA(idx, mod, seg, ofs, data, size)\
	inject_data(idx, mod, seg, ofs, data, size)

static SceUID hook_import(int idx, char *mod, int libnid, int funcnid, void *func) {
	SceUID ret = taiHookFunctionImport(hook_ref+idx, mod, libnid, funcnid, func);
	if (ret >= 0) {
		SCE_DBG_LOG_INFO("Hooked %d UID %08X\n", idx, ret);
		hook_id[idx] = ret;
	} else {
		SCE_DBG_LOG_ERROR("Failed to hook %d error %08X\n", idx, ret);
	}
	return ret;
}
#define HOOK_IMPORT(idx, mod, libnid, funcnid, func)\
	hook_import(idx, mod, libnid, funcnid, func##_hook)

static SceUID hook_offset(int idx, int mod, int ofs, void *func) {
	SceUID ret = taiHookFunctionOffset(hook_ref+idx, mod, 0, ofs, 1, func);
	if (ret >= 0) {
		SCE_DBG_LOG_INFO("Hooked %d UID %08X\n", idx, ret);
		hook_id[idx] = ret;
	} else {
		SCE_DBG_LOG_ERROR("Failed to hook %d error %08X\n", idx, ret);
	}
	return ret;
}
#define HOOK_OFFSET(idx, mod, ofs, func)\
	hook_offset(idx, mod, ofs, func##_hook)

static int UNINJECT(int idx) {
	int ret = 0;
	if (inject_id[idx] >= 0) {
		ret = taiInjectRelease(inject_id[idx]);
		if (ret == 0) {
			SCE_DBG_LOG_INFO("Uninjected %d UID %08X\n", idx, inject_id[idx]);
			inject_id[idx] = -1;
		} else {
			SCE_DBG_LOG_ERROR("Failed to uninject %d UID %08X error %08X\n", idx, inject_id[idx], ret);
		}
	} else {
		SCE_DBG_LOG_WARNING("Tried to uninject %d but not injected\n", idx);
	}
	return ret;
}

static int UNHOOK(int idx) {
	int ret = 0;
	if (hook_id[idx] >= 0) {
		ret = taiHookRelease(hook_id[idx], hook_ref[idx]);
		if (ret == 0) {
			SCE_DBG_LOG_INFO("Unhooked %d UID %08X\n", idx, hook_id[idx]);
			hook_id[idx] = -1;
			hook_ref[idx] = -1;
		} else {
			SCE_DBG_LOG_ERROR("Failed to unhook %d UID %08X error %08X\n", idx, hook_id[idx], ret);
		}
	} else {
		SCE_DBG_LOG_WARNING("Tried to unhook %d but not hooked\n", idx);
	}
	return ret;
}

static SceUID sceKernelAllocMemBlock_hook(char *name, int type, int size, void *opt) {
	static int moved = 0;

#if PATCH_MODE == PATCH_720
	if (moved < 2 && type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW && size == 0x300000) {
		type = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW;
		moved++;
		SCE_DBG_LOG_INFO("moved %d KB from cdram to phycont\n", size / 1024);
#elif PATCH_MODE == PATCH_544_MSAA_4X
	if (moved < 1 && type == SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE && size == 0x800000) {
		type = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW;
		moved++;
		SCE_DBG_LOG_INFO("moved %d KB from main to phycont\n", size / 1024);
#endif
	}
	SCE_DBG_LOG_DEBUG("allocate %08X %08X (%d KB) %s\n", type, size, size / 1024, name);

	SceKernelFreeMemorySizeInfo info;
	info.size = sizeof(info);
	sceKernelGetFreeMemorySize(&info);
	SCE_DBG_LOG_DEBUG("main %d cdram %d phycont %d\n",
		info.size_user / 1024, info.size_cdram / 1024, info.size_phycont / 1024);

	return TAI_NEXT(sceKernelAllocMemBlock_hook, hook_ref[0], name, type, size, opt);
}

#if PATCH_MODE == PATCH_720

static int sceGxmInitialize_hook(SceGxmInitializeParams *params) {
	SCE_DBG_LOG_INFO("parameter buffer reduced %d KB -> %d KB\n",
		params->parameterBufferSize / 1024,
		(params->parameterBufferSize - 0x400000) / 1024);
	params->parameterBufferSize -= 0x400000;
	return TAI_NEXT(sceGxmInitialize_hook, hook_ref[1], params);
}

static int scale_one1_hook(int r0, int r1, int r2, float *r3) {
	r3[1] *= SCALE_X;
	r3[2] *= SCALE_Y;
	return TAI_NEXT(scale_one1_hook, hook_ref[2], r0, r1, r2, r3);
}

static int scale_one2_hook(float s0, float s1, int r0, int r1, int r2) {
	s0 *= SCALE_X;
	s1 *= SCALE_Y;
	return TAI_NEXT(scale_one2_hook, hook_ref[3], s0, s1, r0, r1, r2);
}

static int scale_four1_hook(float x, float y, float w, float h, float s4, int r0, int r1) {
	if (x == 0.0 && y == 0.0 && w == 1280.0 / 960.0 && h == 720.0 / 544.0) {
		SCE_DBG_LOG_TRACE("scale_four1 prescaled\n");
	} else {
		x *= SCALE_X;
		y *= SCALE_Y;
		w *= SCALE_X;
		h *= SCALE_Y;
	}
	return TAI_NEXT(scale_four1_hook, hook_ref[4], x, y, w, h, s4, r0, r1);
}

static int scale_four2_hook(float x, float y, float w, float h, int r0, int r1) {
	if (x == 0.0 && y == 0.0 && w == 1280.0 / 960.0 && h == 720.0 / 544.0) {
		SCE_DBG_LOG_TRACE("scale_four2 prescaled\n");
	} else {
		x *= SCALE_X;
		y *= SCALE_Y;
		w *= SCALE_X;
		h *= SCALE_Y;
	}
	return TAI_NEXT(scale_four2_hook, hook_ref[5], x, y, w, h, r0, r1);
}

#elif PATCH_MODE == PATCH_544_MSAA_4X

static int graphics_init_hook(void *r0) {
	*(int*)(r0 + 0x20 + 0x4 * 0x4) = SCE_GXM_MULTISAMPLE_4X;
	return TAI_NEXT(graphics_init_hook, hook_ref[7], r0);
}

#endif

static int sceDisplaySetFrameBuf_hook(SceDisplayFrameBuf *fb, int mode) {
	static uint32_t start_time = 0;
	static bool failed = false;

	if (!start_time) { start_time = sceKernelGetProcessTimeLow(); }

	if (fb && fb->base) {
		fnblit_set_fb(fb->base, fb->pitch, fb->width, fb->height);

#if PATCH_MODE == PATCH_720
		if (failed) {
			fb->width = 960;
			fb->height = 544;
			fnblit_printf(0, 0, "Catherine Full Body HD Patch failed: 1280x720");
			fnblit_printf(0, 28, "Install Sharpscale and turn on 'Enable Full HD'");
		} else if (sceKernelGetProcessTimeLow() - start_time < 15 * 1000 * 1000) {
			fnblit_printf(0, 0, "Catherine Full Body HD Patch success: 1280x720");
		}
#elif PATCH_MODE == PATCH_544_MSAA_4X
		if (sceKernelGetProcessTimeLow() - start_time < 15 * 1000 * 1000) {
			fnblit_printf(0, 0, "Catherine Full Body HD Patch success: 960x544 MSAA 4x");
		}
#endif

	}

	int ret = TAI_NEXT(sceDisplaySetFrameBuf_hook, hook_ref[6], fb, mode);
	failed = failed || ret < 0;

	return ret;
}

static void startup(void) {
	SCE_DBG_FILELOG_INIT("ux0:/catherinefbhd.log");
	sceClibMemset(inject_id, 0xFF, sizeof(inject_id));
	sceClibMemset(hook_id, 0xFF, sizeof(hook_id));
	sceClibMemset(hook_ref, 0xFF, sizeof(hook_ref));
}

static void cleanup(void) {
	for (int i = 0; i < N_INJECT; i++) { UNINJECT(i); }
	for (int i = 0; i < N_HOOK; i++) { UNHOOK(i); }
	SCE_DBG_FILELOG_TERM();
}

USED int module_start(UNUSED SceSize args, UNUSED const void *argp) {
	startup();

	tai_module_info_t minfo;
	minfo.size = sizeof(minfo);
	GLZ(taiGetModuleInfo(CFB_MOD_NAME, &minfo));

	if (minfo.module_nid != 0x193F08A5) {
		SCE_DBG_LOG_ERROR("Module nid mismatched\n");
		goto fail;
	}

	SceUID cfb_modid = minfo.modid;

	// 3D offscreen buffer

	char *width_patch, *height_patch;

#if PATCH_MODE == PATCH_720
	width_patch = "\x40\xF2\x00\x55"; // mov.w r5, #1280
	height_patch = "\x40\xF2\xD0\x26"; // mov.w r6, #720
#elif PATCH_MODE == PATCH_544_MSAA_4X
	width_patch = "\x4f\xf4\x70\x75"; // mov.w r5, #960
	height_patch = "\x4f\xf4\x08\x76"; // mov.w r6, #544
#endif

	GLZ(INJECT_DATA(0, cfb_modid, 0, 0x0BBE98, width_patch, 4));
	GLZ(INJECT_DATA(1, cfb_modid, 0, 0x0BBEA0, height_patch, 4));

	// main/UI buffer

#if PATCH_MODE == PATCH_720
	// mov.w r0, #1280 (width)
	GLZ(INJECT_DATA(2, cfb_modid, 0, 0x0BBE7A, "\x40\xF2\x00\x50", 4));
	// mov.w r0, #720 (height)
	GLZ(INJECT_DATA(3, cfb_modid, 0, 0x0BBE82, "\x40\xF2\xD0\x20", 4));
	// mov.w r1, #1280 (pitch)
	GLZ(INJECT_DATA(4, cfb_modid, 0, 0x345E64, "\x40\xF2\x00\x51", 4));
	// mov.w r14, #1280 (width)
	// mov.w r12, #720 (height)
	GLZ(INJECT_DATA(5, cfb_modid, 0, 0x345E6A, "\x40\xF2\x00\x5E\x40\xF2\xD0\x2C", 8));

	// scale_loop1 0x8109c4f6 - multiply by 1280, 720

	// movw r12, #1280
	// movw r3, #720
	// nop x 6
	// vmov s0, r12
	GLZ(INJECT_DATA(6, cfb_modid, 0, 0x9C506,
		"\x40\xf2\x00\x5c"
		"\x40\xf2\xd0\x23"
		"\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf"
		"\x00\xee\x10\xca", 24));
	// nop
	// vmov s0, r3
	GLZ(INJECT_DATA(7, cfb_modid, 0, 0x9C532,
		"\x00\xbf"
		"\x00\xee\x10\x3a", 6));

	// scale_loop2 0x8109c558 - multiply by 1280, 720

	// movw r3, #1280
	// movw r0, #720
	// nop x 5
	// vmov s0, r3
	GLZ(INJECT_DATA(8, cfb_modid, 0, 0x9C566,
		"\x40\xf2\x00\x53"
		"\x40\xf2\xd0\x20"
		"\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf"
		"\x00\xee\x10\x3a", 22));
	// nop
	// vmov s0, r0
	GLZ(INJECT_DATA(9, cfb_modid, 0, 0x9C58E,
		"\x00\xbf"
		"\x00\xee\x10\x0a", 6));

	// scale title logos

	// movt r0, #0x44a0
	// movt r1, #0x4434
	GLZ(INJECT_DATA(10, cfb_modid, 0, 0x2E7CAA, "\xc4\xf2\xa0\x40\xc4\xf2\x34\x41", 8));

	GLZ(HOOK_OFFSET(2, cfb_modid, 0x9C43A, scale_one1));
	GLZ(HOOK_OFFSET(3, cfb_modid, 0x9C49C, scale_one2));
	GLZ(HOOK_OFFSET(4, cfb_modid, 0x9C5BC, scale_four1));
	GLZ(HOOK_OFFSET(5, cfb_modid, 0x9C688, scale_four2));
#endif

	// memory management

	GLZ(HOOK_IMPORT(0, CFB_MOD_NAME, 0x37FE725A, 0xB9D5EBDE, sceKernelAllocMemBlock));

#if PATCH_MODE == PATCH_720
	GLZ(HOOK_IMPORT(1, CFB_MOD_NAME, 0xF76B66BD, 0xB0F1E4EC, sceGxmInitialize));
#endif

	// multisample antialiasing

#if PATCH_MODE == PATCH_544_MSAA_4X
	GLZ(HOOK_OFFSET(7, cfb_modid, 0x2F29C0, graphics_init));
#endif

	// on-screen display

	fnblit_set_font(_binary_font_sfn_start);
	fnblit_set_fg(0xFFFFFFFF);
	fnblit_set_bg(0x00000000);
	GLZ(HOOK_IMPORT(6, CFB_MOD_NAME, 0x4FAACD11, 0x7A410B64, sceDisplaySetFrameBuf));

	SCE_DBG_LOG_INFO("module_start success\n");
	return SCE_KERNEL_START_SUCCESS;

fail:
	SCE_DBG_LOG_ERROR("module_start failed\n");
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

USED int module_stop(UNUSED SceSize args, UNUSED const void *argp) {
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
