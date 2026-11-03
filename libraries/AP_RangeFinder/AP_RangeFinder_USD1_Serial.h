    AP_RangeFinder_USD1_Serial(AP_RangeFinder &frontend, AP_SerialManager &serial_manager);

    // flag that indicates we have already found the frame header (0x48)
    // This flag must persist between calls to get_reading() because a
    // complete frame may be split across multiple reads.
    bool _hdr_found;
