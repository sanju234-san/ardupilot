    : AP_RangeFinder_Backend(frontend),
      _serial_manager(serial_manager),
      _linebuf_len(0),
      _hdr_found(false)
    uint8_t c;
    // The required number of bytes for a complete frame depends on the protocol version.
    // V0 (legacy) uses a 3‑byte frame: [0x48][high][low]
    // V1 (current) uses a 6‑byte frame: [0x48][0x4E][high][low][checksum][0x0A]
    const uint8_t required_len = (_version == 0) ? 3 : sizeof(_linebuf);

    while (read_uart(c)) {
        // If we have not yet found a header, look for it.
        if (!_hdr_found) {
            if (c == 0x48) {
                _hdr_found = true;
                _linebuf_len = 0;
                _linebuf[_linebuf_len++] = c;
            }
            // discard any non‑header bytes
            continue;
        }

        // Header already found – accumulate bytes.
        if (_linebuf_len < sizeof(_linebuf)) {
            _linebuf[_linebuf_len++] = c;
        }

        // If we have collected enough bytes for the expected frame length, attempt decode.
        if (_linebuf_len >= required_len) {
            // Verify header for V1 (second byte must be 0x4E). If it is not, resynchronise.
            if (_version != 0 && _linebuf[1] != 0x4E) {
                // Bad sync – shift buffer left by one and keep searching for a proper header.
                memmove(_linebuf, _linebuf + 1, --_linebuf_len);
                // If the shifted byte is a header, keep _hdr_found true, otherwise reset.
                if (_linebuf[0] != 0x48) {
                    _hdr_found = false;
                }
                continue;
            }

            // Decode distance.
            if (_version == 0) {
                // V0: bytes 1 and 2 hold the distance (big‑endian cm)
                reading_cm = static_cast<float>((_linebuf[1] << 8) | _linebuf[2]);
            } else {
                // V1: bytes 2 and 3 hold the distance (big‑endian cm)
                reading_cm = static_cast<float>((_linebuf[2] << 8) | _linebuf[3]);
            }
            // Reset state for next frame.
            _hdr_found = false;
            _linebuf_len = 0;
            return true;
        }
    }
    // No complete frame yet.
    return false;
