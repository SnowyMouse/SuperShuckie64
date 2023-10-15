#ifndef REPLAY_RECORDER_H
#define REPLAY_RECORDER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RR_ReplayWriter RR_ReplayWriter;
typedef struct RR_ReplayReader RR_ReplayReader;
typedef struct RR_ReplayReaderItem RR_ReplayReaderItem;
typedef struct RR_ReplayReaderItemCollection RR_ReplayReaderItemCollection;

typedef struct RR_String32 {
    char string[32];
} RR_String32;

typedef enum RR_PacketType {
    RR_CustomData = 0,
    RR_LoadSRAM = 1,
    RR_ChangeGameSpeed = 2,
    RR_Bookmark = 3,
    RR_SetInput8 = 4,
    RR_SetInput16 = 5,
    RR_SetInput32 = 6,
    RR_SetInput64 = 7,
    RR_SetInputData8 = 8,
    RR_SetInputData16 = 9,
    RR_SetInputData32 = 10,
    RR_SetInputData64 = 11,
    RR_AddSaveState = 12,
    RR_LoadSaveState = 13,
    RR_NoOp = 255,
} RR_PacketType;

RR_ReplayWriter *RR_ReplayWriter_new(const char *emulator_info, const char *rom_name, const void *rom_data, size_t rom_data_size, const void *bios_data, size_t bios_data_size);
void RR_ReplayWriter_free(RR_ReplayWriter *writer);
void RR_ReplayWriter_get_stream(const RR_ReplayWriter *writer, const void **stream, size_t *length);
void RR_ReplayWriter_next_frame(RR_ReplayWriter *writer);
void RR_ReplayWriter_write_SetInput8(RR_ReplayWriter *writer, uint8_t input);
void RR_ReplayWriter_write_SetInput16(RR_ReplayWriter *writer, uint16_t input);
void RR_ReplayWriter_write_SetInput32(RR_ReplayWriter *writer, uint32_t input);
void RR_ReplayWriter_write_SetInput64(RR_ReplayWriter *writer, uint64_t input);
void RR_ReplayWriter_write_SetInputData8(RR_ReplayWriter *writer, const void *input, size_t input_length);
void RR_ReplayWriter_write_SetInputData16(RR_ReplayWriter *writer, const void *input, size_t input_length);
void RR_ReplayWriter_write_SetInputData32(RR_ReplayWriter *writer, const void *input, size_t input_length);
void RR_ReplayWriter_write_SetInputData64(RR_ReplayWriter *writer, const void *input, size_t input_length);
void RR_ReplayWriter_write_LoadSRAM(RR_ReplayWriter *writer, const void *data, size_t data_length);
void RR_ReplayWriter_write_Bookmark(RR_ReplayWriter *writer, RR_String32 *name);
void RR_ReplayWriter_write_CustomData(RR_ReplayWriter *writer, RR_String32 *name, const void *data, size_t data_length);
void RR_ReplayWriter_write_ChangeGameSpeed(RR_ReplayWriter *writer, uint16_t speed);
void RR_ReplayWriter_write_AddSaveState(RR_ReplayWriter *writer, uint32_t index, const void *data, size_t data_length);
void RR_ReplayWriter_write_LoadSaveState(RR_ReplayWriter *writer, uint32_t index);

RR_ReplayReader *RR_ReplayReader_new(const void *stream_data, size_t stream_data_len);
void RR_ReplayReader_free(RR_ReplayReader *reader);
const RR_ReplayReaderItem *RR_ReplayReader_next(RR_ReplayReader *reader, bool *error);
RR_ReplayReaderItemCollection *RR_ReplayReader_collect(RR_ReplayReader *reader, bool *error);

void RR_ReplayReaderItemCollection_free(RR_ReplayReaderItemCollection *collection);
size_t RR_ReplayReaderItemCollection_len(RR_ReplayReaderItemCollection *collection);
const RR_ReplayReaderItem *RR_ReplayReaderItemCollection_get_n(RR_ReplayReaderItemCollection *collection);

uint8_t RR_ReplayReaderItem_get_packet_type(const RR_ReplayReaderItem *item);
uint8_t RR_ReplayReaderItem_get_delay(const RR_ReplayReaderItem *item);
void RR_ReplayReaderItem_read_SetInput8(const RR_ReplayReaderItem *item, uint8_t *input);
void RR_ReplayReaderItem_read_SetInput16(const RR_ReplayReaderItem *item, uint16_t *input);
void RR_ReplayReaderItem_read_SetInput32(const RR_ReplayReaderItem *item, uint32_t *input);
void RR_ReplayReaderItem_read_SetInput64(const RR_ReplayReaderItem *item, uint64_t *input);
void RR_ReplayReaderItem_read_SetInputData8(const RR_ReplayReaderItem *item, const void **input, size_t *input_length);
void RR_ReplayReaderItem_read_SetInputData16(const RR_ReplayReaderItem *item, const void **input, size_t *input_length);
void RR_ReplayReaderItem_read_SetInputData32(const RR_ReplayReaderItem *item, const void **input, size_t *input_length);
void RR_ReplayReaderItem_read_SetInputData64(const RR_ReplayReaderItem *item, const void **input, size_t *input_length);
void RR_ReplayReaderItem_read_LoadSRAM(RR_ReplayWriter *writer, const void **data, size_t *data_length);
void RR_ReplayReaderItem_read_Bookmark(const RR_ReplayReaderItem *item, RR_String32 *name);
void RR_ReplayReaderItem_read_CustomData(const RR_ReplayReaderItem *item, RR_String32 *name, const void **data, size_t *data_length);
void RR_ReplayReaderItem_read_ChangeGameSpeed(const RR_ReplayReaderItem *item, uint16_t *speed);
void RR_ReplayReaderItem_read_AddSaveState(const RR_ReplayReaderItem *item, uint32_t *index, const void **data, size_t *data_length);
void RR_ReplayReaderItem_read_LoadSaveState(const RR_ReplayReaderItem *item, uint32_t *index);

#ifdef __cplusplus
}
#endif

#endif
