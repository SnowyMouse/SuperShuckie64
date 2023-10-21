#ifndef REPLAY_RECORDER_HPP
#define REPLAY_RECORDER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <memory>

#if __cplusplus >= 202002
#define USE_SPANS 1
#include <span>
#else
#define USE_SPANS 0
#endif

#include "replay_recorder.h"

static_assert(sizeof(std::byte) == sizeof(std::uint8_t));

class ReplayWriter {
public:
    ReplayWriter(const char *emulator_info, const char *rom_name, const void *rom_data, std::size_t rom_data_size, const void *bios_data, std::size_t bios_data_size) noexcept :
        writer(RR_ReplayWriter_new(emulator_info, rom_name, rom_data, rom_data_size, bios_data, bios_data_size), RR_ReplayWriter_free) {}

    std::vector<std::byte> get_stream() const noexcept {
        std::size_t length;
        const std::byte *data;
        RR_ReplayWriter_get_stream(this->writer.get(), reinterpret_cast<const void **>(&data), &length);
        return std::vector<std::byte>(data, data + length);
    }

    void next_frame() noexcept {
        RR_ReplayWriter_next_frame(this->writer.get());
    }

    void write_SetInput8(std::uint8_t input) noexcept {
        RR_ReplayWriter_write_SetInput8(this->writer.get(), input);
    }

    void write_SetInput16(std::uint16_t input) noexcept {
        RR_ReplayWriter_write_SetInput16(this->writer.get(), input);
    }

    void write_SetInput32(std::uint32_t input) noexcept {
        RR_ReplayWriter_write_SetInput32(this->writer.get(), input);
    }

    void write_SetInput64(std::uint64_t input) noexcept {
        RR_ReplayWriter_write_SetInput64(this->writer.get(), input);
    }

    void write_SetInputData8(const void *input, std::size_t input_length) noexcept {
        RR_ReplayWriter_write_SetInputData8(this->writer.get(), input, input_length);
    }

    void write_SetInputData16(const void *input, std::size_t input_length) noexcept {
        RR_ReplayWriter_write_SetInputData16(this->writer.get(), input, input_length);
    }

    void write_SetInputData32(const void *input, std::size_t input_length) noexcept {
        RR_ReplayWriter_write_SetInputData32(this->writer.get(), input, input_length);
    }

    void write_SetInputData64(const void *input, std::size_t input_length) noexcept {
        RR_ReplayWriter_write_SetInputData64(this->writer.get(), input, input_length);
    }

    void write_Bookmark(const RR_String32 &bookmark) noexcept {
        RR_ReplayWriter_write_Bookmark(this->writer.get(), &bookmark);
    }

    void write_CustomData(const RR_String32 &name, const void *data, std::size_t data_length) noexcept {
        RR_ReplayWriter_write_CustomData(this->writer.get(), &name, data, data_length);
    }

    void write_ChangeGameSpeed(std::uint16_t speed) noexcept {
        RR_ReplayWriter_write_ChangeGameSpeed(this->writer.get(), speed);
    }

    void write_AddSaveState(std::uint32_t index, const void *data, std::size_t data_length) noexcept {
        RR_ReplayWriter_write_AddSaveState(this->writer.get(), index, data, data_length);
    }

    void write_LoadSaveState(std::uint32_t index) noexcept {
        RR_ReplayWriter_write_LoadSaveState(this->writer.get(), index);
    }

    void write_WriteRAMByteAddr32(std::uint8_t byte, std::uint32_t offset) {
        RR_ReplayWriter_write_WriteRAMByteAddr32(this->writer.get(), byte, offset);
    }

    void write_WriteRAMByteAddr64(std::uint8_t byte, std::uint64_t offset) {
        RR_ReplayWriter_write_WriteRAMByteAddr64(this->writer.get(), byte, offset);
    }

    void write_WriteROMByteOffset32(std::uint8_t byte, std::uint32_t offset) {
        RR_ReplayWriter_write_WriteROMByteOffset32(this->writer.get(), byte, offset);
    }

    void write_WriteROMByteOffset64(std::uint8_t byte, std::uint64_t offset) {
        RR_ReplayWriter_write_WriteROMByteOffset64(this->writer.get(), byte, offset);
    }

private:
    std::unique_ptr<RR_ReplayWriter, void(*)(RR_ReplayWriter *)> writer;
};

class ReplayReaderItem {
public:
    ReplayReaderItem(const RR_ReplayReaderItem *item) noexcept : item(item) {}

    std::uint8_t get_delay() const noexcept {
        return RR_ReplayReaderItem_get_delay(this->item);
    }

    RR_PacketType get_packet_type() const noexcept {
        return static_cast<RR_PacketType>(RR_ReplayReaderItem_get_packet_type(this->item));
    }

    void read_SetInput8(std::uint8_t &input) const noexcept {
        RR_ReplayReaderItem_read_SetInput8(this->item, &input);
    }

    void read_SetInput16(std::uint16_t &input) const noexcept {
        RR_ReplayReaderItem_read_SetInput16(this->item, &input);
    }

    void read_SetInput32(std::uint32_t &input) const noexcept {
        RR_ReplayReaderItem_read_SetInput32(this->item, &input);
    }

    void read_SetInput64(std::uint64_t &input) const noexcept {
        RR_ReplayReaderItem_read_SetInput64(this->item, &input);
    }

    void read_SetInputData8(const void *&input, std::size_t &input_length) const noexcept {
        RR_ReplayReaderItem_read_SetInputData8(this->item, &input, &input_length);
    }

    void read_SetInputData16(const void *&input, std::size_t &input_length) const noexcept {
        RR_ReplayReaderItem_read_SetInputData16(this->item, &input, &input_length);
    }

    void read_SetInputData32(const void *&input, std::size_t &input_length) const noexcept {
        RR_ReplayReaderItem_read_SetInputData32(this->item, &input, &input_length);
    }

    void read_SetInputData64(const void *&input, std::size_t &input_length) const noexcept {
        RR_ReplayReaderItem_read_SetInputData64(this->item, &input, &input_length);
    }

    void read_Bookmark(RR_String32 &bookmark) const noexcept {
        RR_ReplayReaderItem_read_Bookmark(this->item, &bookmark);
    }

    void read_CustomData(RR_String32 &name, const void *&data, std::size_t &data_length) const noexcept {
        RR_ReplayReaderItem_read_CustomData(this->item, &name, &data, &data_length);
    }

    void read_ChangeGameSpeed(std::uint16_t &speed) const noexcept {
        RR_ReplayReaderItem_read_ChangeGameSpeed(this->item, &speed);
    }

    void read_AddSaveState(std::uint32_t &index, const void *&data, std::size_t &data_length) const noexcept {
        RR_ReplayReaderItem_read_AddSaveState(this->item, &index, &data, &data_length);
    }

    void read_LoadSaveState(std::uint32_t &index) const noexcept {
        RR_ReplayReaderItem_read_LoadSaveState(this->item, &index);
    }

    void read_WriteRAMByteAddr32(std::uint8_t &byte, std::uint32_t &offset) const noexcept {
        RR_ReplayReaderItem_read_WriteRAMByteAddr32(this->item, &byte, &offset);
    }

    void read_WriteRAMByteAddr64(std::uint8_t &byte, std::uint64_t &offset) const noexcept {
        RR_ReplayReaderItem_read_WriteRAMByteAddr64(this->item, &byte, &offset);
    }

    void read_WriteROMByteOffset32(std::uint8_t &byte, std::uint32_t &offset) const noexcept {
        RR_ReplayReaderItem_read_WriteROMByteOffset32(this->item, &byte, &offset);
    }

    void read_WriteROMByteOffset64(std::uint8_t &byte, std::uint64_t &offset) const noexcept {
        RR_ReplayReaderItem_read_WriteROMByteOffset64(this->item, &byte, &offset);
    }

    #if USE_SPANS == 1

    #define MAKE_SPAN(start, size) std::span(reinterpret_cast<const std::byte *>(start), reinterpret_cast<const std::byte *>(start) + size)

    #define MAKE_SET_INPUT_DATA(fn) \
        const void *input_bytes; \
        std::size_t input_length; \
        fn(this->item, &input_bytes, &input_length); \
        input = MAKE_SPAN(input_bytes, input_length);

    void read_SetInputData8(std::span<const std::byte> &input) const noexcept {
        MAKE_SET_INPUT_DATA(RR_ReplayReaderItem_read_SetInputData8);
    }

    void read_SetInputData16(std::span<const std::byte> &input) const noexcept {
        MAKE_SET_INPUT_DATA(RR_ReplayReaderItem_read_SetInputData16);
    }

    void read_SetInputData32(std::span<const std::byte> &input) const noexcept {
        MAKE_SET_INPUT_DATA(RR_ReplayReaderItem_read_SetInputData32);
    }

    void read_SetInputData64(std::span<const std::byte> &input) const noexcept {
        MAKE_SET_INPUT_DATA(RR_ReplayReaderItem_read_SetInputData64);
    }

    #undef MAKE_SET_INPUT_DATA

    void read_AddSaveState(std::uint32_t &index, std::span<const std::byte> &data) const noexcept {
        const void *save_state;
        std::size_t save_state_length;
        RR_ReplayReaderItem_read_AddSaveState(this->item, &index, &save_state, &save_state_length);
        data = MAKE_SPAN(save_state, save_state_length);
    }

    void read_CustomData(RR_String32 &name, std::span<const std::byte> &data) const noexcept {
        const void *custom_data;
        std::size_t custom_data_length;
        RR_ReplayReaderItem_read_CustomData(this->item, &name, &custom_data, &custom_data_length);
        data = MAKE_SPAN(custom_data, custom_data_length);
    }

    #undef MAKE_SPAN
    #endif

private:
    const RR_ReplayReaderItem *item;
};

class ReplayReaderItemCollection {
public:
    ReplayReaderItemCollection(RR_ReplayReaderItemCollection *collection) noexcept : collection(collection, RR_ReplayReaderItemCollection_free) {}

    std::size_t len() const noexcept {
        return RR_ReplayReaderItemCollection_len(this->collection.get());
    }

    ReplayReaderItem get_n(std::size_t index) const noexcept {
        return ReplayReaderItem(RR_ReplayReaderItemCollection_get_n(this->collection.get(), index));
    }

    ReplayReaderItem operator[](std::size_t index) const noexcept {
        return this->get_n(index);
    }

private:
    std::unique_ptr<RR_ReplayReaderItemCollection, void(*)(RR_ReplayReaderItemCollection *)> collection;
};

class ReplayReader {
public:
    ReplayReader(const void *stream_data, std::size_t stream_data_len) noexcept :
        reader(RR_ReplayReader_new(stream_data, stream_data_len), RR_ReplayReader_free) {}

    std::optional<ReplayReaderItem> next(bool &error) noexcept {
        auto *item_maybe = RR_ReplayReader_next(this->reader.get(), &error);
        if(item_maybe) {
            return ReplayReaderItem(item_maybe);
        }
        return std::nullopt;
    }

    std::optional<ReplayReaderItemCollection> collect(bool &error) noexcept {
        auto *collection_maybe = RR_ReplayReader_collect(this->reader.get(), &error);
        if(collection_maybe) {
            return ReplayReaderItemCollection(collection_maybe);
        }
        return std::nullopt;
    }

private:
    std::unique_ptr<RR_ReplayReader, void(*)(RR_ReplayReader *)> reader;
};

#undef USE_SPANS

#endif
