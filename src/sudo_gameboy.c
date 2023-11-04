// I solumnly swear I am up to no good
#define GB_INTERNAL

#include <gb.h>
#include <stdint.h>

typedef struct BankData {
    uint16_t addr_start;
    uint16_t max_bank;
    uint16_t current_bank;
    size_t len;
    uint8_t *start;
    uint8_t *requested_byte_location;
} BankData;

static void resolve_byte_for_bank(BankData *bank, uint16_t requested_bank_or_ffff, uint16_t address) {
    size_t actual_bank;
    if(requested_bank_or_ffff > bank->max_bank) {
        actual_bank = bank->current_bank;
    }
    else {
        actual_bank = requested_bank_or_ffff;
    }

    size_t bank_offset = actual_bank * bank->len;
    size_t byte_offset = (address - bank->addr_start);

    bank->requested_byte_location = bank->start + bank_offset + byte_offset;
}

static void get_bank_data(GB_gameboy_t *gb, uint16_t address, BankData *bank, uint16_t requested_bank_or_ffff) {
    // Cartridge ROM.
    if(address < 0x8000) {
        bank->max_bank = gb->rom_size / 0x4000;
        bank->len = 0x4000;
        bank->start = gb->rom;

        if(address < 0x4000) {
            bank->addr_start = 0x0000;
            bank->current_bank = gb->mbc_rom0_bank & (bank->max_bank - 1);
        }
        else if(address < 0x8000) {
            bank->addr_start = 0x4000;
            bank->current_bank = gb->mbc_rom_bank & (bank->max_bank - 1);
        }

        return resolve_byte_for_bank(bank, requested_bank_or_ffff, address);
    }

    // VRAM
    if(address < 0xA000) {
        bank->addr_start = 0x8000;
        bank->max_bank = GB_is_cgb(gb) ? 1 : 0;
        bank->len = 0x2000;
        bank->start = gb->vram;
        bank->current_bank = gb->cgb_vram_bank;

        return resolve_byte_for_bank(bank, requested_bank_or_ffff, address);
    }

    // Cartridge RAM
    if(address < 0xC000) {
    echo_ram:
        bank->len = 0x1000;
        bank->start = gb->mbc_ram;
        bank->current_bank = gb->mbc_ram_bank;

        if(address < 0xB000) {
            bank->addr_start = 0xA000;
            bank->max_bank = 0;
        }
        else {
            bank->addr_start = 0xB000;
            bank->max_bank = 0;
        }

        return resolve_byte_for_bank(bank, requested_bank_or_ffff, address);
    }

    // WRAM
    if(address < 0xE000) {
        bank->len = 0x1000;
        bank->start = gb->ram;

        if(address < 0xD000) {
            bank->addr_start = 0xC000;
            bank->max_bank = 0;
            bank->current_bank = 0;
        }
        else {
            bank->addr_start = 0xD000;
            bank->max_bank = gb->ram_size / bank->len;
            bank->current_bank = gb->cgb_ram_bank;
        }

        return resolve_byte_for_bank(bank, requested_bank_or_ffff, address);
    }

    // ECHO RAM
    if(address < 0xFE00) {
        address = (address & (0x1FFF)) + 0xC000;
        goto echo_ram;
    }

    // OAM
    if(address < 0xFEA0) {
        bank->addr_start = 0xFE00;
        bank->max_bank = 0;
        bank->len = 0xA0;
        bank->start = gb->oam;
        bank->current_bank = 0;
        bank->requested_byte_location = bank->start + (address - bank->addr_start);
        return;
    }

    // Null
    if(address < 0xFF00) {
        bank->addr_start = 0xFEA0;
        bank->max_bank = 0;
        bank->len = 0;
        bank->start = NULL;
        bank->current_bank = 0;
        bank->requested_byte_location = NULL;
        return;
    }

    // I/O
    if(address < 0xFF80) {
        bank->addr_start = 0xFF00;
        bank->max_bank = 0;
        bank->len = 0x80;
        bank->start = gb->io_registers;
        bank->current_bank = 0;
        bank->requested_byte_location = bank->start + (address - bank->addr_start);
        return;
    }

    // HRAM
    if(address < 0xFFFF) {
        bank->addr_start = 0xFF80;
        bank->max_bank = 0;
        bank->len = 0x7F;
        bank->start = gb->hram;
        bank->current_bank = 0;
        bank->requested_byte_location = bank->start + (address - bank->addr_start);
        return;
    }

    // IE
    bank->addr_start = 0xFFFF;
    bank->max_bank = 0;
    bank->len = 1;
    bank->start = &gb->interrupt_enable;
    bank->current_bank = 0;
    bank->requested_byte_location = bank->start;
}

uint8_t GB_safe_read_memory_except_its_actually_safe(GB_gameboy_t *gb, uint16_t address, uint16_t bank_or_ffff) {
    BankData bank_data;
    get_bank_data(gb, address, &bank_data, bank_or_ffff);

    return bank_data.requested_byte_location ? *bank_data.requested_byte_location : 0xFF;
}
