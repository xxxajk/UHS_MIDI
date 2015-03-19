/*
 *******************************************************************************
 * USB-MIDI class driver for USB Host Shield 3.0 Library
 *
 * Based on:
 * USB-MIDI class driver for USB Host Shield 2.0 Library
 * Copyright 2012-2014 Yuuichi Akagawa
 *
 * Idea from LPK25 USB-MIDI to Serial MIDI converter
 *   by Collin Cunningham - makezine.com, narbotic.com
 *
 * for use with USB Host Shield 2.0 from Circuitsathome.com
 * https://github.com/felis/USB_Host_Shield_2.0
 *******************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *******************************************************************************
 */

#if !defined(_UHS_MIDI_H_)
#define _UHS_MIDI_H_
#define MIDI_MAX_ENDPOINTS 3 //endpoint 0, bulk_IN(MIDI), bulk_OUT(MIDI), bulk_IN(VSP), bulk_OUT(VSP)
#define MIDI_EVENT_BUFFER_SIZE (64*4)

class UHS_MIDI : public UHS_USBInterface {
protected:
        static const uint8_t epDataInIndex; // DataIn endpoint index(MIDI)
        static const uint8_t epDataOutIndex; // DataOUT endpoint index(MIDI)
        static const uint8_t epDataInIndexVSP; // DataIn endpoint index(Vendor Specific Protocol)
        static const uint8_t epDataOutIndexVSP; // DataOUT endpoint index(Vendor Specific Protocol)



        /* MIDI Event packet buffer */
        uint8_t readPtr;
        uint16_t rcvd;

        unsigned int countSysExDataSize(uint8_t *dataptr);
#ifdef DEBUG
        void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);
#endif
private:
        uint8_t lookupMsgSize(uint8_t midiMsg);

public:
        uint8_t recvBuf[MIDI_EVENT_BUFFER_SIZE];
        /* Endpoint data structure */
        volatile UHS_EpInfo epInfo[MIDI_MAX_ENDPOINTS];

        uint16_t pid, vid;
        UHS_MIDI(UHS_USB_HOST_BASE *p);
        bool Polling(void) {
                return bPollEnable;
        };

        virtual uint8_t GetAddress() {
                return bAddress;
        };

        // Configure and internal methods, these should never be called by a user's sketch.
        uint8_t Start(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        void Release(void);
        void Poll(void);
        void DriverDefaults(void);

        uint8_t RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr);
        // Methods for receiving and sending data
        uint8_t Read(void);
        int Write(uint8_t count, uint8_t *q);

};
#ifdef LOAD_USB_MIDI
#include "UHS_MIDI_INLINE.h"
#endif

#endif //_UHS_MIDI_H_
