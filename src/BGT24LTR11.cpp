#include "BGT24LTR11.h"

template <class T>
void BGT24LTR11<T>::init(T& serialPort) {
    _serial = &serialPort;
}

template <class T>
uint16_t BGT24LTR11<T>::calculateChecksum(uint16_t *data, uint16_t data_length) {
    uint16_t checksum = 0;
    for (uint16_t i = 0;  i < data_length; ++i) {
        checksum += data[i];
    }

    return checksum;
}

template <class T>
uint16_t BGT24LTR11<T>::messageChecksum(uint16_t high_order, uint16_t low_order) {
    return ((high_order << 8) + low_order);
}

template <class T>
uint8_t BGT24LTR11<T>::getInfo(uint16_t* target_tate, uint16_t* speed) {
    uint16_t data[7] = {0};

    // clear rx buffer (good for sequential processing)
    while (_serial->available() > 0) {
        _serial->read();
    }

    _serial->write(commandC1, 7);

    // ensures the data is written to the wire, flushes also rx buffer again
    _serial->flush();

    // there might be a small timewindow where no data is available (module is processing request and preparing response) so we wait until data is present
    const uint32_t start_time = micros();
    while ((_serial->available() == 0) && ((unsigned long)(micros() - start_time) < 5000));

    // as long as data is available, seek through it and try to parse the response datagram
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_TARGET) {
                    for (int i = 0; i < 7; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_TARGET + calculateChecksum(data, 5);
                    const uint16_t msg_checksum = messageChecksum(data[5], data[6]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    *speed = (data[2] * 256 + data[3]);
                    *target_tate = data[4];

                    return 1;
                }
            }
        }
    }
    return 0;
}

template <class T>
uint16_t BGT24LTR11<T>::getSpeed() {
    uint16_t data[7] = {0};
    _serial->write(commandC1, 7);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_TARGET) {
                    for (int i = 0; i < 7; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_TARGET + calculateChecksum(data, 5);
                    const uint16_t msg_checksum = messageChecksum(data[5], data[6]);

                    if (checksum != msg_checksum) {
                        return -1;
                    }

                    return (data[2] * 256 + data[3]);
                }
            }
        }
        /* code */
    }
    return -1;
}

template <class T>
uint16_t BGT24LTR11<T>::getTargetState() {
    uint16_t data[7] = {0};
    _serial->write(commandC1, 7);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_TARGET) {
                    for (int i = 0; i < 7; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_TARGET + calculateChecksum(data, 5);
                    const uint16_t msg_checksum = messageChecksum(data[5], data[6]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    //2 --> target approach
                    //1 --> target leave
                    //0 --> Not Found target
                    return data[4];
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::getIQADC(uint16_t I_d[], uint16_t Q_d[], uint16_t* len) {
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_IQADC) {
                    uint16_t length_h, length_l, length;
                    length_h = _serial->read();
                    length_l = _serial->read();
                    length = length_h * 256 + length_l;
                    for (int i = 0; i < ((length - 2) / 2); i++) {
                        I_d[i] = _serial->read();
                        Q_d[i] = _serial->read();
                    }
                    *len = (length - 2) / 2;
                    return 1;
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::setSpeedScope(uint16_t maxspeed, uint16_t minspeed) {
    uint16_t data[8] = {0};
    uint16_t len = 0;
    unsigned char commandC3[11] = {0x55, 0x2A, 0xC3, 0x00, 0x06, 0x02, 0x09, 0x01, 0x03, 0x01, 0x57};
    if (maxspeed < minspeed) {
        return 0;
    }
    commandC3[5] = maxspeed / 256;
    commandC3[6] = maxspeed % 256;
    commandC3[7] = minspeed / 256;
    commandC3[8] = minspeed % 256;
    for (int i = 0; i < 9; i++) {
        len += commandC3[i];
    }
    commandC3[9] = len / 256;
    commandC3[10] = len % 256;
    _serial->write(commandC3, 11);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_SET_SPEED_SCOPE) {
                    for (int i = 0; i < 8; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_SET_SPEED_SCOPE + calculateChecksum(data, 6);
                    const uint16_t msg_checksum = messageChecksum(data[6], data[7]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    if (((data[2] * 256 + data[3]) == maxspeed) && ((data[4] * 256 + data[5]) == minspeed)) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::getSpeedScope(uint16_t* maxspeed, uint16_t* minspeed) {
    unsigned char commandC4[7] = {0x55, 0x2A, 0xC4, 0x00, 0x02, 0x01, 0x45};
    uint16_t data[8] = {0};
    _serial->write(commandC4, 7);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_SPEED_SCOPE) {
                    for (int i = 0; i < 8; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_SPEED_SCOPE + calculateChecksum(data, 6);
                    const uint16_t msg_checksum = messageChecksum(data[6], data[7]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    *maxspeed = data[2] * 256 + data[3];
                    *minspeed = data[4] * 256 + data[5];
                    return 1;
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::setMode(uint16_t mode) {
    if (mode > 1) {
        return 0;
    }
    unsigned char commandC5[8] = {0x55, 0x2A, 0xC5, 0x00, 0x03, 0x00, 0x01, 0x47};
    if (mode == 1) {
        commandC5[5] = 0x01;
        commandC5[7] = 0x48;
    }
    uint16_t data[5] = {0};
    _serial->write(commandC5, 8);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_SET_MODE) {
                    for (int i = 0; i < 5; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_SET_MODE + calculateChecksum(data, 3);
                    const uint16_t msg_checksum = messageChecksum(data[3], data[4]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    if (data[2] == mode) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::getMode() {
    unsigned char commandC6[7] = {0x55, 0x2A, 0xC6, 0x00, 0x02, 0x01, 0x47};
    uint16_t data[5] = {0};
    _serial->write(commandC6, 7);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_MODE) {
                    for (int i = 0; i < 5; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_MODE + calculateChecksum(data, 3);
                    const uint16_t msg_checksum = messageChecksum(data[3], data[4]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    //return 1 ---> detect the target mode
                    //return 2 ---> Reported I/Q ADC
                    //return 0 ---> fail
                    if (data[2] == 0) {
                        return 1;
                    } else if (data[2] == 1) {
                        return 2;
                    } else {
                        return 0;
                    }
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint8_t BGT24LTR11<T>::setThreshold(uint16_t threshold) {
    int len = 0;
    uint16_t data[8] = {0};
    unsigned char commandC7[11] = {0x55, 0x2A, 0xC7, 0x00, 0x06, 0x00, 0x00, 0x04, 0x00, 0x01, 0x50};
    commandC7[5] = threshold / 256 / 256 / 256 % 256;
    commandC7[6] = threshold / 256 / 256 % 256;
    commandC7[7] = threshold / 256 % 256;
    commandC7[8] = threshold % 256;
    for (int i = 0; i < 9; i++) {
        len += commandC7[i];
    }
    commandC7[9] = len / 256 % 256;
    commandC7[10] = len % 256;
    _serial->write(commandC7, 11);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_SET_THRESHOLD) {
                    for (int i = 0; i < 8; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_SET_THRESHOLD + calculateChecksum(data, 6);
                    const uint16_t msg_checksum = messageChecksum(data[6], data[7]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    uint16_t thr = data[2] * 256 * 256 * 256 + data[3] * 256 * 256 + data[4] * 256 + data[5];
                    if (thr == threshold) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            }
        }
        /* code */
    }
    return 0;
}

template <class T>
uint16_t BGT24LTR11<T>::getThreshold() {
    unsigned char commandC8[7] = {0x55, 0x2A, 0xC8, 0x00, 0x02, 0x01, 0x49};
    uint16_t data[8] = {0};
    _serial->write(commandC8, 7);
    while (_serial->available() > 0) {
        if (_serial->read() == BGT24LTR11_MESSAGE_HEAD) {
            if (_serial->read() == BGT24LTR11_SEND_ADDRESS) {
                if (_serial->read() == BGT24LTR11_COMMAND_GET_THRESHOLD) {
                    for (int i = 0; i < 8; i++) {
                        data[i] = _serial->read();
                    }

                    const uint16_t checksum = BGT24LTR11_MESSAGE_HEAD + BGT24LTR11_SEND_ADDRESS + BGT24LTR11_COMMAND_GET_THRESHOLD + calculateChecksum(data, 6);
                    const uint16_t msg_checksum = messageChecksum(data[6], data[7]);

                    if (checksum != msg_checksum) {
                        return 0;
                    }

                    return data[2] * 256 * 256 * 256 + data[3] * 256 * 256 + data[4] * 256 + data[5];
                }
            }
        }
        /* code */
    }
    return 0;
}

#if defined(ARDUINO_SAMD_VARIANT_COMPLIANCE) || defined(NRF52840_XXAA) || defined(SEEED_XIAO_M0)
    template class BGT24LTR11<Uart>;
#endif
template class BGT24LTR11<HardwareSerial>;

#if defined(__AVR__) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350) || defined(ARDUINO_ARCH_RENESAS)
    #include <SoftwareSerial.h>
    template class BGT24LTR11<SoftwareSerial>;
#endif
