#include "gb.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>

static inline void GB_iowrite_gbc(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	assert(GB_is_system_gbc(gb) == GB_TRUE);

	switch (addr & 0x7F) {
		case 0x4D: // (KEY1)
			IO_KEY1 = value & 1;
			break;

		case 0x4F: // (VBK)
			IO_VBK = value & 1;
			GB_update_vram_banks(gb);
			break;

		case 0x50: // unused (bootrom?)
			break;

		case 0x51: // (HDMA1)
			IO_HDMA1 = value;
			break;

		case 0x52: // (HDMA2)
			IO_HDMA2 = value;
			break;

		case 0x53: // (HMDA3)
			IO_HDMA3 = value;
			break;

		case 0x54: // (HDMA4)
			IO_HDMA4 = value;
			break;

		case 0x55: // (HDMA5)
			GB_hdma5_write(gb, value);
			break;

		case 0x68: // BCPS
			IO_BCPS = value;
			break;

		case 0x69: // BCPD
			GB_bcpd_write(gb, value);
			IO_BCPD = value;
			break;

		case 0x6A: // OCPS
			IO_OCPS = value;
			break;

		case 0x6B: // OCPD
			GB_ocpd_write(gb, value);
			IO_OCPD = value;
			break;

		case 0x70: // (SVBK) always set between 1-7
			IO_SVBK = (value & 0x07) + ((value & 0x07) == 0x00);
			GB_update_wram_banks(gb);
			break;
	}
}

GB_U8 GB_ioread(struct GB_Data* gb, GB_U16 addr) {
	switch (addr & 0x7F) {
		case 0x00:
			return GB_joypad_get(gb);

		case 0x01:
			return GB_serial_sb_read(gb);

		case 0x4D:
			if (GB_is_system_gbc(gb) == GB_TRUE) {
				return gb->cpu.double_speed > 0 ? 0x80 : 0x00;
			} else {
				return 0xFF;
			}

		case 0x55: // HDMA5
			if (GB_is_system_gbc(gb) == GB_TRUE) {
				return GB_hdma5_read(gb);
			} else {
				return 0xFF;
			}

		default:
			return IO[addr & 0x7F];
	}
}

void GB_iowrite(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	switch (addr & 0x7F) {
		case 0x00: // joypad
			IO_JYP = value;
			break;

		case 0x01: // SB (Serial transfer data)
			IO_SB = value;
			break;

		case 0x02: // SC (Serial Transfer Control)
			GB_serial_sc_write(gb, value);
			break;

		case 0x04:
			IO_DIV_UPPER = IO_DIV_LOWER = 0;
			break;

		case 0x05:
			IO_TIMA = value;
			break;

		case 0x06:
			IO_TMA = value;
			break;

		case 0x07:
			IO_TAC = (value | 248);
			break;
		
		case 0x0F:
			IO_IF = (value | 224);
			break;

		case 0x10:
			IO_NR10 = value | 0x80;
			break;

		case 0x11:
			IO_NR11 = value;
			break;

		case 0x12:
			IO_NR12 = value;
			break;

		case 0x13:
			IO_NR13 = value;
			break;

		case 0x14:
			IO_NR14 = value;
			break;

		case 0x16:
			IO_NR21 = value;
			break;

		case 0x17:
			IO_NR22 = value;
			break;

		case 0x18:
			IO_NR23 = value;
			break;

		case 0x19:
			IO_NR24 = value;
			break;

		case 0x1A:
			IO_NR30 = value | 0x7F;
			break;

		case 0x1B:
			IO_NR31 = value;
			break;

		case 0x1C:
			IO_NR32 = value | 159;
			break;

		case 0x1D:
			IO_NR33 = value;
			break;

		case 0x1E:
			IO_NR34 = value;
			break;

		case 0x20:
			IO_NR41 = value | 0xC0;
			break;

		case 0x21:
			IO_NR42 = value;
			break;

		case 0x22:
			IO_NR43 = value;
			break;

		case 0x23:
			IO_NR44 = value | 0x3F;
			break;

		case 0x24:
			IO_NR50 = value;
			break;

		case 0x25:
			IO_NR51 = value;
			break;

		case 0x26:
			IO_NR52 = value & 0x80;
			break;

		case 0x40:
			IO_LCDC = value;
			break;

		case 0x41:
			IO_STAT = (IO_STAT & 0x7) | (value & (120)) | 0x80;
			break;

		case 0x42:
			IO_SCY = value;
			break;

		case 0x43:
			IO_SCX = value;
			break;

		case 0x45:
			IO_LYC = value;
			break;

		case 0x46:
			IO_DMA = value;
			GB_DMA(gb);
			break;

		case 0x47:
			gb->ppu.dirty_bg[0] |= (IO_BGP != value);
			IO_BGP = value;
			break;

		case 0x48:
			gb->ppu.dirty_obj[0] |= (IO_OBP0 != value);
			IO_OBP0 = value;
			break;

		case 0x49:
			gb->ppu.dirty_obj[1] |= (IO_OBP1 != value);
			IO_OBP1 = value;
			break;

		case 0x4A:
			IO_WY = value;
			break;

		case 0x4B:
			IO_WX = value;
			break;

		default:
			if (GB_is_system_gbc(gb) == GB_TRUE) {
				GB_iowrite_gbc(gb, addr, value);
			}
			break;
	}
}

GB_U8 GB_read8(struct GB_Data* gb, const GB_U16 addr) {
	if (LIKELY(addr < 0xFE00)) {
		return gb->mmap[(addr >> 12)][addr & 0x0FFF];
	} else if (addr <= 0xFE9F) {
        return gb->ppu.oam[addr & 0x9F];
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return GB_ioread(gb, addr);
    } else if (addr >= 0xFF80) {
        return gb->hram[addr & 0x7F];
    }

	// unused section in address area.
	return 0xFF;
}

void GB_write8(struct GB_Data* gb, GB_U16 addr, GB_U8 value) {
	if (LIKELY(addr < 0xFE00)) {
		switch ((addr >> 12) & 0xF) {
			case 0x0: case 0x1: case 0x2: case 0x3: case 0x4:
			case 0x5: case 0x6: case 0x7: case 0xA: case 0xB:
				gb->cart.write(gb, addr, value);
				break;
			
			case 0x8: case 0x9:
				gb->ppu.vram[IO_VBK][addr & 0x1FFF] = value;
				break;
			
			case 0xC: case 0xE:
				gb->wram[0][addr & 0x0FFF] = value;
				break;
			
			case 0xD: case 0xF:
				gb->wram[IO_SVBK][addr & 0x0FFF] = value;
				break;
		}
	} else if (addr <= 0xFE9F) {
        gb->ppu.oam[addr & 0x9F] = value;
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        GB_iowrite(gb, addr, value);
    } else if (addr >= 0xFF80) {
        gb->hram[addr & 0x7F] = value;
    }
}

GB_U16 GB_read16(struct GB_Data* gb, GB_U16 addr) {
	const GB_U8 lo = GB_read8(gb, addr);
	const GB_U8 hi = GB_read8(gb, addr + 1);
	return (hi << 8) | lo;
}

void GB_write16(struct GB_Data* gb, GB_U16 addr, GB_U16 value) {
	GB_write8(gb, addr + 0, value & 0xFF);
    GB_write8(gb, addr + 1, value >> 8);
}

void GB_update_rom_banks(struct GB_Data* gb) {
	const GB_U8* rom_bank0 = gb->cart.get_rom_bank(gb, 0);
	const GB_U8* rom_bankx = gb->cart.get_rom_bank(gb, 1);
	gb->mmap[0x0] = rom_bank0 + 0x0000;
	gb->mmap[0x1] = rom_bank0 + 0x1000;
	gb->mmap[0x2] = rom_bank0 + 0x2000;
	gb->mmap[0x3] = rom_bank0 + 0x3000;

	gb->mmap[0x4] = rom_bankx + 0x0000;
	gb->mmap[0x5] = rom_bankx + 0x1000;
	gb->mmap[0x6] = rom_bankx + 0x2000;
	gb->mmap[0x7] = rom_bankx + 0x3000;
}

void GB_update_ram_banks(struct GB_Data* gb) {
	const GB_U8* cart_ram = gb->cart.get_ram_bank(gb, 0);
	gb->mmap[0xA] = cart_ram + 0x0000;
	gb->mmap[0xB] = cart_ram + 0x1000;
}

void GB_update_vram_banks(struct GB_Data* gb) {
	if (GB_is_system_gbc(gb) == GB_TRUE) {
		gb->mmap[0x8] = gb->ppu.vram[IO_VBK] + 0x0000;
		gb->mmap[0x9] = gb->ppu.vram[IO_VBK] + 0x1000;
	} else {
		gb->mmap[0x8] = gb->ppu.vram[0] + 0x0000;
		gb->mmap[0x9] = gb->ppu.vram[0] + 0x1000;
	}
}

void GB_update_wram_banks(struct GB_Data* gb) {
	gb->mmap[0xC] = gb->wram[0];
	gb->mmap[0xE] = gb->wram[0];

	if (GB_is_system_gbc(gb) == GB_TRUE) {
		gb->mmap[0xD] = gb->wram[IO_SVBK];
		gb->mmap[0xF] = gb->wram[IO_SVBK];
	} else {
		gb->mmap[0xD] = gb->wram[1];
		gb->mmap[0xF] = gb->wram[1];
	}
}

void GB_setup_mmap(struct GB_Data* gb) {
	GB_update_rom_banks(gb);
	GB_update_ram_banks(gb);
	GB_update_vram_banks(gb);
	GB_update_wram_banks(gb);
}
