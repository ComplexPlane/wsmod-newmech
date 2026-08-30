#include "cardio.h"

#include "arena.h"
#include "modlink.h"

namespace cardio {

namespace {

// Corresponds to CARD call we're waiting for
enum class WriteState {
    Idle,
    Mount,
    Create,  // If memcard file doesn't exist
    Delete,  // Else, it exists but it's too small, delete and create from scratch
    Write,
};

struct WriteRequest {
    // Params
    const char* file_name;
    Slot slot;
    const void* buf;
    u32 buf_size;
    void (*callback)(mkb::CARDResult);

    // State
    WriteState state;
    mkb::CARDFileInfo card_file_info;
    u32 write_size;  // Multiples of card sector size
};

u8* s_card_work_area;
WriteRequest s_curr_write;  // Current params
WriteRequest s_next_write;  // Params for use for next write
bool s_write_requested;

/*
 * Probably not Nintendo-approved hack for letting us read/write to the same savefile even if the
 * gamecode varies. Just modify the gamecode (stored at 0x80000000) to GM2E8P (vanilla SMB2) before
 * doing memcard operations!
 */

mkb::CARDResult read_file_internal(arena::Arena* arena, const char* file_name, Slot slot,
                                   void** out_buf) {
    mkb::CARDResult res = mkb::CARD_RESULT_READY;
    s32 chan = static_cast<s32>(slot);
    mkb::CARDFileInfo card_file_info = {};

    // Probe and mount card
    mkb::CARDProbeEx(chan, nullptr, nullptr);
    mkb::CARDMountAsync(chan, s_card_work_area, nullptr, nullptr);
    do {
        res = mkb::CARDGetResultCode(chan);
    } while (res == mkb::CARD_RESULT_BUSY);
    if (res != mkb::CARD_RESULT_READY) {
        return res;
    }

    // Open file
    res = mkb::CARDOpen(chan, const_cast<char*>(file_name), &card_file_info);
    if (res != mkb::CARD_RESULT_READY) {
        mkb::CARDUnmount(chan);
        return res;
    }

    // Get file size
    mkb::CARDStat stat = {};
    res = mkb::CARDGetStatus(chan, card_file_info.fileNo, &stat);
    if (res != mkb::CARD_RESULT_READY) {
        mkb::CARDUnmount(chan);
        return res;
    }

    u32 orig_arena_occupied = arena->get_occupied();
    u32 buf_size = (stat.length + mkb::CARD_READ_SIZE - 1) & ~(mkb::CARD_READ_SIZE - 1);
    void* buf = arena->alloc_bytes(buf_size, 32);
    if (buf == nullptr) {
        // Not quite the right error (we're out of memory, not out of card space)
        mkb::CARDUnmount(chan);
        return mkb::CARD_RESULT_INSSPACE;
    }

    mkb::CARDReadAsync(&card_file_info, buf, buf_size, 0, nullptr);
    do {
        res = mkb::CARDGetResultCode(chan);
    } while (res == mkb::CARD_RESULT_BUSY);
    if (res != mkb::CARD_RESULT_READY) {
        arena->restore_occupied(orig_arena_occupied);
        mkb::CARDUnmount(chan);
        return res;
    }

    *out_buf = buf;
    return mkb::CARD_RESULT_READY;
}

void finish_write(mkb::CARDResult res) {
    // I'm assuming that trying to unmount when mounting failed is OK
    mkb::CARDUnmount(static_cast<s32>(s_curr_write.slot));
    s_curr_write.callback(res);
}

void tick_write() {
    s32 chan = static_cast<s32>(s_curr_write.slot);

    switch (s_curr_write.state) {
        case WriteState::Idle: {
            if (s_write_requested) {
                // Kick off write operation
                s_curr_write = s_next_write;
                s_write_requested = false;

                // Probe and begin mounting card
                s32 sector_size;
                mkb::CARDProbeEx(chan, nullptr, &sector_size);
                s_curr_write.write_size =
                    (s_curr_write.buf_size + sector_size - 1) & ~(sector_size - 1);
                mkb::CARDMountAsync(chan, s_card_work_area, nullptr, nullptr);
                s_curr_write.state = WriteState::Mount;
            }
            break;
        }

        case WriteState::Mount: {
            mkb::CARDResult res = mkb::CARDGetResultCode(chan);
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    // Try to open the file
                    res = mkb::CARDOpen(chan, const_cast<char*>(s_curr_write.file_name),
                                        &s_curr_write.card_file_info);
                    if (res == mkb::CARD_RESULT_READY) {
                        // Check if file is too small
                        mkb::CARDStat stat = {};
                        res = mkb::CARDGetStatus(chan, s_curr_write.card_file_info.fileNo, &stat);
                        if (res != mkb::CARD_RESULT_READY) {
                            finish_write(res);

                        } else if (stat.length < s_curr_write.write_size) {
                            // Recreate file
                            mkb::CARDFastDeleteAsync(chan, s_curr_write.card_file_info.fileNo,
                                                     nullptr);
                            s_curr_write.state = WriteState::Delete;

                        } else {
                            // Card opened successfully, proceed directly to writing
                            mkb::CARDWriteAsync(&s_curr_write.card_file_info,
                                                const_cast<void*>(s_curr_write.buf),
                                                s_curr_write.write_size, 0, nullptr);
                            s_curr_write.state = WriteState::Write;
                        }

                    } else if (res == mkb::CARD_RESULT_NOFILE) {
                        // Create new file
                        mkb::CARDCreateAsync(chan, const_cast<char*>(s_curr_write.file_name),
                                             s_curr_write.write_size, &s_curr_write.card_file_info,
                                             nullptr);
                        s_curr_write.state = WriteState::Create;

                    } else {
                        // Some other error, fail entire write operation
                        finish_write(res);
                    }

                } else {
                    // Error mounting
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Create: {
            mkb::CARDResult res = mkb::CARDGetResultCode(chan);
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    mkb::CARDWriteAsync(&s_curr_write.card_file_info,
                                        const_cast<void*>(s_curr_write.buf),
                                        s_curr_write.write_size, 0, nullptr);
                    s_curr_write.state = WriteState::Write;
                } else {
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Delete: {
            mkb::CARDResult res = mkb::CARDGetResultCode(chan);
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    mkb::CARDCreateAsync(chan, const_cast<char*>(s_curr_write.file_name),
                                         s_curr_write.write_size, &s_curr_write.card_file_info,
                                         nullptr);
                    s_curr_write.state = WriteState::Create;
                } else {
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Write: {
            mkb::CARDResult res = mkb::CARDGetResultCode(chan);
            if (res != mkb::CARD_RESULT_BUSY) {
                // Either succeeded or failed, either way we're done
                finish_write(res);
            }
            break;
        }
    }
}

}  // namespace

mkb::CARDResult read_file(arena::Arena* arena, const char* file_name, Slot slot, void** out_buf) {
    return read_file_internal(arena, file_name, slot, out_buf);
}

void write_file(const char* file_name, Slot slot, const void* buf, u32 buf_size,
                void (*callback)(mkb::CARDResult)) {
    s_next_write = {
        .file_name = file_name,
        .slot = slot,
        .buf = buf,
        .buf_size = buf_size,
        .callback = callback,
    };
    s_write_requested = true;
}

void init(arena::Arena* arena) {
    s_card_work_area = (u8*)arena->alloc_bytes(mkb::CARD_WORKAREA_SIZE, 32);
    modlink::set_card_work_area(s_card_work_area);
}

void tick() {
    tick_write();
}

}  // namespace cardio
