/*
This file is part of Catherine Full Body 720p Patch for PSTV
Copyright 2020 浅倉麗子

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

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <taihen.h>
#include <fnblit.h>

int vsnprintf(char *buf, SceSize n, const char *fmt, va_list arg) {
	return sceClibVsnprintf(buf, n, fmt, arg);
}

extern char font_sfn[];
extern int font_sfn_len;

#define SCALE_W (1280.0 / 960.0)
#define SCALE_H (720.0 / 540.0)
#define PARAMETER_BUFFER_SIZE (SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE - 0x400000)

#define FEQF(a, b) (fabsf((a) - (b)) < 0.00005)

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while (0)

__attribute__ ((__format__ (__printf__, 1, 2)))
static void LOG(const char *fmt, ...) {
	(void)fmt;

	#ifdef LOG_PRINTF
	sceClibPrintf("\033[0;36m[CatherineFB720p]\033[0m ");
	va_list args;
	va_start(args, fmt);
	sceClibVprintf(fmt, args);
	va_end(args);
	#endif
}

#define N_INJECT 10
static SceUID inject_id[N_INJECT];

#define N_HOOK 7
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

static SceUID INJECT_DATA(int idx, int mod, int seg, int ofs, void *data, int size) {
	inject_id[idx] = taiInjectData(mod, seg, ofs, data, size);
	LOG("Injected %d UID %08X\n", idx, inject_id[idx]);
	return inject_id[idx];
}

static SceUID hook_import(int idx, char *mod, int libnid, int funcnid, void *func) {
	hook_id[idx] = taiHookFunctionImport(hook_ref+idx, mod, libnid, funcnid, func);
	LOG("Hooked %d UID %08X\n", idx, hook_id[idx]);
	return hook_id[idx];
}
#define HOOK_IMPORT(idx, mod, libnid, funcnid, func)\
	hook_import(idx, mod, libnid, funcnid, func##_hook)

static SceUID hook_offset(int idx, int mod, int ofs, void *func) {
	hook_id[idx] = taiHookFunctionOffset(hook_ref+idx, mod, 0, ofs, 1, func);
	LOG("Hooked %d UID %08X\n", idx, hook_id[idx]);
	return hook_id[idx];
}
#define HOOK_OFFSET(idx, mod, ofs, func)\
	hook_offset(idx, mod, ofs, func##_hook)

static int UNINJECT(int idx) {
	int ret = 0;
	if (inject_id[idx] >= 0) {
		ret = taiInjectRelease(inject_id[idx]);
		LOG("Uninjected %d UID %08X\n", idx, inject_id[idx]);
		inject_id[idx] = -1;
	}
	return ret;
}

static int UNHOOK(int idx) {
	int ret = 0;
	if (hook_id[idx] >= 0) {
		ret = taiHookRelease(hook_id[idx], hook_ref[idx]);
		LOG("Unhooked %d UID %08X\n", idx, hook_id[idx]);
		hook_id[idx] = -1;
		hook_ref[idx] = -1;
	}
	return ret;
}

static SceUID sceKernelAllocMemBlock_hook(char *name, int type, int size, void *opt) {
	static int moved = 0;
	if (moved < 2 && type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW && size == 0x300000) {
		type = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW;
		moved++;
		LOG("moved %d KB from cdram to phycont\n", size / 1024);
	}
	LOG("allocate %08X %08X (%d KB) %s\n", type, size, size / 1024, name);
	return TAI_NEXT(sceKernelAllocMemBlock_hook, hook_ref[0], name, type, size, opt);
}

static int sceGxmInitialize_hook(SceGxmInitializeParams *params) {
	LOG("parameter buffer reduced %d KB -> %d KB\n",
		params->parameterBufferSize / 1024,
		PARAMETER_BUFFER_SIZE / 1024);
	params->parameterBufferSize = PARAMETER_BUFFER_SIZE;
	return TAI_NEXT(sceGxmInitialize_hook, hook_ref[1], params);
}

static int scale_one1_hook(int r0, int r1, int r2, int r3) {
	*(float*)(r3 + 0x4) *= SCALE_W;
	*(float*)(r3 + 0x8) *= SCALE_H;
	return TAI_NEXT(scale_one1_hook, hook_ref[2], r0, r1, r2, r3);
}

static int scale_one2_hook(float s0, float s1, int r0, int r1, int r2) {
	s0 *= SCALE_W;
	s1 *= SCALE_H;
	return TAI_NEXT(scale_one2_hook, hook_ref[3], s0, s1, r0, r1, r2);
}

static int scale_four1_hook(float x, float y, float w, float h, float s4, int r0, int r1) {
	if (FEQF(x, 0.0) && FEQF(y, 0.0) && FEQF(w, 1280.0 / 960.0) && FEQF(h, 720.0 / 544.0)) {
		LOG("scale_four1 prescaled\n");
		goto done;
	}
	x *= SCALE_W;
	y *= SCALE_H;
	w *= SCALE_W;
	h *= SCALE_H;
done:
	return TAI_NEXT(scale_four1_hook, hook_ref[4], x, y, w, h, s4, r0, r1);
}

static int scale_four2_hook(float x, float y, float w, float h, int r0, int r1) {
	if (FEQF(x, 0.0) && FEQF(y, 0.0) && FEQF(w, 1280.0 / 960.0) && FEQF(h, 720.0 / 544.0)) {
		LOG("scale_four2 prescaled\n");
		goto done;
	}
	x *= SCALE_W;
	y *= SCALE_H;
	w *= SCALE_W;
	h *= SCALE_H;
done:
	return TAI_NEXT(scale_four2_hook, hook_ref[5], x, y, w, h, r0, r1);
}

static int sceDisplaySetFrameBuf_hook(SceDisplayFrameBuf *fb, int mode) {
	static uint32_t start_time = 0;
	static bool failed = false;

	if (!start_time) { start_time = sceKernelGetProcessTimeLow(); }

	if (fb && fb->base) {
		fnblit_set_fb(fb->base, fb->pitch, fb->width, fb->height);
		if (failed) {
			fb->width = 960;
			fb->height = 544;
			fnblit_printf(0, 0, "Catherine Full Body 1280x720 render failed");
			fnblit_printf(0, 28, "Install Sharpscale and turn on 'Enable Full HD'");
		} else {
			fnblit_printf(0, 0, "Catherine Full Body 1280x720 render success");
		}
	}

	int ret = TAI_NEXT(sceDisplaySetFrameBuf_hook, hook_ref[6], fb, mode);

	if (!failed && fb && fb->base && fb->pitch == 1280 && fb->width == 1280 && fb->height == 720) {
		failed = ret < 0;
	}

	if (!failed && sceKernelGetProcessTimeLow() - start_time > 15 * 1000 * 1000) {
		UNHOOK(6);
	}

	return ret;
}

static void startup(void) {
	sceClibMemset(inject_id, 0xFF, sizeof(inject_id));
	sceClibMemset(hook_id, 0xFF, sizeof(hook_id));
	sceClibMemset(hook_ref, 0xFF, sizeof(hook_ref));
}

static void cleanup(void) {
	for (int i = 0; i < N_INJECT; i++) { UNINJECT(i); }
	for (int i = 0; i < N_HOOK; i++) { UNHOOK(i); }
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	startup();

	tai_module_info_t minfo;
	minfo.size = sizeof(minfo);
	GLZ(taiGetModuleInfo("xrd758_psp2", &minfo));

	if (minfo.module_nid != 0x193F08A5) {
		LOG("Module nid mismatched\n");
		goto fail;
	}

	// internal buffer

	// mov.w r5, #1280 (width)
	GLZ(INJECT_DATA(0, minfo.modid, 0, 0x000BBE98, "\x40\xF2\x00\x55", 4));
	// mov.w r6, #720 (height)
	GLZ(INJECT_DATA(1, minfo.modid, 0, 0x000BBEA0, "\x40\xF2\xD0\x26", 4));

	// frame buffer

	// mov.w r0, #1280 (width)
	GLZ(INJECT_DATA(2, minfo.modid, 0, 0x000BBE7A, "\x40\xF2\x00\x50", 4));
	// mov.w r0, #720 (height)
	GLZ(INJECT_DATA(3, minfo.modid, 0, 0x000BBE82, "\x40\xF2\xD0\x20", 4));
	// mov.w r1, #1280 (pitch)
	GLZ(INJECT_DATA(4, minfo.modid, 0, 0x00345E64, "\x40\xF2\x00\x51", 4));
	// mov.w r14, #1280 (width)
	// mov.w r12, #720 (height)
	GLZ(INJECT_DATA(5, minfo.modid, 0, 0x00345E6A, "\x40\xF2\x00\x5E\x40\xF2\xD0\x2C", 8));

	// scale_loop1 0x8109c4f6 - multiply by 1280, 720

	// movw r12, #1280
	// movw r3, #720
	// nop x 6
	// vmov s0, r12
	GLZ(INJECT_DATA(6, minfo.modid, 0, 0x9c506,
		"\x40\xf2\x00\x5c"
		"\x40\xf2\xd0\x23"
		"\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf"
		"\x00\xee\x10\xca", 24));
	// nop
	// vmov s0, r3
	GLZ(INJECT_DATA(7, minfo.modid, 0, 0x9c532,
		"\x00\xbf"
		"\x00\xee\x10\x3a", 6));

	// scale_loop2 0x8109c558 - multiply by 1280, 720

	// movw r3, #1280
	// movw r0, #720
	// nop x 5
	// vmov s0, r3
	GLZ(INJECT_DATA(8, minfo.modid, 0, 0x9c566,
		"\x40\xf2\x00\x53"
		"\x40\xf2\xd0\x20"
		"\x00\xbf\x00\xbf\x00\xbf\x00\xbf\x00\xbf"
		"\x00\xee\x10\x3a", 22));
	// nop
	// vmov s0, r0
	GLZ(INJECT_DATA(9, minfo.modid, 0, 0x9c58e,
		"\x00\xbf"
		"\x00\xee\x10\x0a", 6));

	GLZ(HOOK_IMPORT(0, "xrd758_psp2", 0x37FE725A, 0xB9D5EBDE, sceKernelAllocMemBlock));
	GLZ(HOOK_IMPORT(1, "xrd758_psp2", 0xF76B66BD, 0xB0F1E4EC, sceGxmInitialize));

	GLZ(HOOK_OFFSET(2, minfo.modid, 0x9c43a, scale_one1));
	GLZ(HOOK_OFFSET(3, minfo.modid, 0x9c49c, scale_one2));
	GLZ(HOOK_OFFSET(4, minfo.modid, 0x9c5bc, scale_four1));
	GLZ(HOOK_OFFSET(5, minfo.modid, 0x9c688, scale_four2));

	fnblit_set_font(font_sfn);
	fnblit_set_fg(0xFFFFFFFF);
	fnblit_set_bg(0x00000000);
	GLZ(HOOK_IMPORT(6, "xrd758_psp2", 0x4FAACD11, 0x7A410B64, sceDisplaySetFrameBuf));

	return SCE_KERNEL_START_SUCCESS;

fail:
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
