#ifndef UDP_COMMAND_HANDLER_H
#define UDP_COMMAND_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RA_UDPCommandHandler RA_UDPCommandHandler;

typedef struct SizedPtr {
    const uint8_t *byteptr;
    uint64_t size;
} SizedPtr;

enum RA_RequestType {
    RA_RequestType_Invalid = 0,
    RA_RequestType_ReadCoreMemory,
    RA_RequestType_WriteCoreMemory
};

RA_UDPCommandHandler *RA_UDP_CMD_new(void);
void RA_UDP_CMD_free(RA_UDPCommandHandler *);
void RA_UDP_CMD_get_request_data(const RA_UDPCommandHandler *handler, uint8_t *type, uint64_t *addr, SizedPtr *param);
void RA_UDP_CMD_pop_request(RA_UDPCommandHandler *handler);
void RA_UDP_CMD_handle_read_request(const RA_UDPCommandHandler *handler, const uint8_t *bytes);

#ifdef __cplusplus
}

#include <cstdint>
#include <memory>

class UDPCommandHandler {
public:
    static std::shared_ptr<UDPCommandHandler> try_new() {
        auto h = std::unique_ptr<RA_UDPCommandHandler, void(*)(RA_UDPCommandHandler *)>(RA_UDP_CMD_new(), RA_UDP_CMD_free);
        if(h.get()) {
            return std::make_shared<UDPCommandHandler>(std::move(h));
        }
        return nullptr;
    }

    void get_request_data(std::uint8_t &type, std::uint64_t &addr, SizedPtr &param) const noexcept {
        RA_UDP_CMD_get_request_data(this->handler.get(), &type, &addr, &param);
    }

    void pop_request() noexcept {
        RA_UDP_CMD_pop_request(this->handler.get());
    }

    void handle_read_request(const std::uint8_t *bytes) const noexcept {
        RA_UDP_CMD_handle_read_request(this->handler.get(), bytes);
    }

    UDPCommandHandler() = delete;
    UDPCommandHandler(std::unique_ptr<RA_UDPCommandHandler, void(*)(RA_UDPCommandHandler *)> &&handler) : handler(std::move(handler)) {}
private:
    std::unique_ptr<RA_UDPCommandHandler, void(*)(RA_UDPCommandHandler *)> handler;
};

#endif

#endif
