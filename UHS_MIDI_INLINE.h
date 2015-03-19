/*
 *******************************************************************************
 * USB-MIDI class driver for USB Host Shield 3.0 Library
 * The difference is that messages are not converted. Subclass to do that.
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

#if defined(_UHS_MIDI_H_) && !defined(UHS_USB_MIDI_LOADED)
#define UHS_USB_MIDI_LOADED
#include <string.h>

const uint8_t UHS_MIDI::epDataInIndex = 1;
const uint8_t UHS_MIDI::epDataOutIndex = 2;

/**
 * Clear all EP data
 * For driver use only.
 */
void UHS_NI UHS_MIDI::DriverDefaults(void) {
        pUsb->DeviceDefaults(MIDI_MAX_ENDPOINTS, this);
}

/**
 * Releases (shuts down) the interface driver.
 * For driver use only.
 */
void UHS_NI UHS_MIDI::Release(void) {
        OnRelease();
        DriverDefaults();
        return;
}

/**
 * Polls the interface.
 * For driver use only.
 */
void UHS_NI UHS_MIDI::Poll(void) {
        OnPoll();
        //long pnt = (long)(millis() - qNextPollTime);
        //if(pnt >= 0L) {
        // TO-DO: allow this to auto-adjust/user adjust, using pnt value
        //        qNextPollTime = millis(); // set ahead, give OnPoll 1ms to do stuff
        //        OnPoll();
        //}
};

/**
 * Indicates that this interface driver supports USB-MIDI to the host enumerator.
 * For driver use only.
 *
 * TO-DO: Add MIDI VID:PID of devices that claim to be using Vendor Specific
 * Interfaces, but are actually in fact USB-MIDI.
 *
 * @param ei Enumeration information
 * @return true if this interface driver can handle this interface description
 */
bool UHS_NI UHS_MIDI::OKtoEnumerate(ENUMERATION_INFO *ei) {
        //printf("UHS_MIDI: checking numep %i, klass %2.2x, subklass %2.2x\r\n", ei->interface.numep, ei->klass, ei->subklass);
        //printf("UHS_MIDI: checking protocol %2.2x, interface.klass %2.2x, interface.subklass %2.2x\r\n", ei->protocol, ei->interface.klass, ei->interface.subklass);
        //printf("UHS_MIDI: checking interface.protocol %2.2x\r\n", ei->interface.protocol);

        return (
                ((ei->klass == UHS_USB_CLASS_AUDIO) || (ei->interface.klass == UHS_USB_CLASS_AUDIO)) &&
                ((ei->subklass == UHS_USB_SUBCLASS_MIDISTREAMING) || (ei->interface.subklass == UHS_USB_SUBCLASS_MIDISTREAMING))

                );
}

/**
 * Sets up interface pipes.
 * For driver use only.
 *
 * @param ei Enumeration information
 * @return 0 always
 */
uint8_t UHS_NI UHS_MIDI::SetInterface(ENUMERATION_INFO *ei) {
        uint8_t index;

        bAddress = ei->address;
        USBTRACE("UHS_MIDI SetInterface\r\n");
        // Fill in the endpoint info structure
        for(uint8_t ep = 0; ep < ei->interface.numep; ep++) {

                //HOST_DEBUG("ep: 0x%2.2x bmAttributes: 0x%2.2x ", ep, ei->interface.epInfo[ep].bmAttributes);
                if(ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_BULK) {
                        index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epDataInIndex : epDataOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = (uint8_t)(ei->interface.epInfo[ep].wMaxPacketSize);
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = USB_NAK_NOWAIT;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                }
        }
        bNumEP = MIDI_MAX_ENDPOINTS;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;

        return 0;
};

/**
 * Start the driver
 * For driver use only.
 *
 * @return 0 for success
 */
uint8_t UHS_NI UHS_MIDI::Start(void) {
        uint8_t rcode = 0;
        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
        if(!rcode) {
                rcode = OnStart();
        }
        if(rcode) {
                Release();
        } else {
                readPtr = 0;
                rcvd = 0;
                memset(recvBuf, 0, MIDI_EVENT_BUFFER_SIZE);
                qNextPollTime = millis() + 100;
                bPollEnable = true;
        }
        return rcode;
}

/**
 * Constructor
 *
 * @param p Pointer to host driver
 */
UHS_MIDI::UHS_MIDI(UHS_USB_HOST_BASE *p) {
        pUsb = p;
        bPollEnable = false;
        if(pUsb) {
                bNumEP = MIDI_MAX_ENDPOINTS;
                DriverDefaults();
                pUsb->RegisterDeviceClass(this);
        }
}

/**
 * Receives One raw USB data packet from MIDI device
 * @param bytes_rcvd set to size expected, returns actual byte count read
 * @param dataptr pointer to 64byte USB data buffer
 * @return 0 on success
 */
uint8_t UHS_NI UHS_MIDI::RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr) {
        uint8_t rcode;
        pUsb->DisablePoll();
        rcode = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
        pUsb->EnablePoll();
        return rcode;
}

/**
 * Transmits 32bit USB-MIDI packets to MIDI interface.
 *
 * @param count how many 32bit packets, Maximum of 16.
 * @param q pointer to 32bit USB-MIDI message buffers.
 * @return how many 32bit packets sent. Negative numbers indicate error.
 */
int UHS_NI UHS_MIDI::Write(uint8_t count, uint8_t *q) {
        pUsb->DisablePoll();
        if(count > 16) count = 16;
        int c = count;
        uint16_t x = count * 4;
        if(x) {
                uint8_t rcode = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, x, q);
                if(rcode) {
                        if(rcode == hrNAK) {
                                c = 0;
                        } else {
                                c = -rcode;
                        }
                }
        }
        pUsb->EnablePoll();
        return c;
}

/**
 * Receives One 32-bit USB-MIDI packet from MIDI interface
 *
 * @param q pointer to One 32bit USB-MIDI message buffer.
 * @return 0 for no data, 1,2,3 bytes of actual data, Negative numbers are error.
 */
uint8_t UHS_NI UHS_MIDI::Read(void) {
        pUsb->DisablePoll();
        uint8_t rcode = hrNAK; //return code
        if(bPollEnable) {
                rcvd = epInfo[epDataInIndex].maxPktSize;
                rcode = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, (uint16_t *) & rcvd, recvBuf);
        }
        pUsb->EnablePoll();
        return rcode;
}

/* look up a MIDI message size from spec */
/*Return                                 */
/*  0 : undefined message                */

/*  0<: Vaild message size(1-3)          */
static const uint8_t msgsztbl[16] = {0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};

/**
 * Verify legal message, return size.
 * @param midiMsg first byte of a 32-bit USB-MIDI packet
 * @return 0 = invalid, else length of message (One to three bytes)
 */
uint8_t UHS_NI UHS_MIDI::lookupMsgSize(uint8_t midiMsg) {
        uint8_t msgSize = 0;

        uint8_t middec = (midiMsg & 0x0fU);
        return (msgsztbl[middec]);
        /*
                //midiMsg
                switch(middec) {
                        case 0x05:
                        case 0x0f:
                                msgSize = 1;
                                break;

                        case 0x02:
                        case 0x06:
                        case 0x0c:
                        case 0x0d:
                                msgSize = 2;
                                break;

                        case 0x03:
                        case 0x04:
                        case 0x07:
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0e:
                                msgSize = 3;
                                break;
                                //case 0x00:
                                //case 0x01:
                        default:
                                break;
                }
                return msgSize;

         */
}


#ifdef DEBUG

void UHS_NI UHS_MIDI::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR * ep_ptr) {
        Notify(PSTR("Endpoint descriptor:"));
        Notify(PSTR("\r\nLength:\t\t"));
        PrintHex<uint8_t>(ep_ptr->bLength);
        Notify(PSTR("\r\nType:\t\t"));
        PrintHex<uint8_t>(ep_ptr->bDescriptorType);
        Notify(PSTR("\r\nAddress:\t"));
        PrintHex<uint8_t>(ep_ptr->bEndpointAddress);
        Notify(PSTR("\r\nAttributes:\t"));
        PrintHex<uint8_t>(ep_ptr->bmAttributes);
        Notify(PSTR("\r\nMaxPktSize:\t"));
        PrintHex<uint16_t>(ep_ptr->wMaxPacketSize);
        Notify(PSTR("\r\nPoll Intrv:\t"));
        PrintHex<uint8_t>(ep_ptr->bInterval);
        Notify(PSTR("\r\n"));
}
#endif



#endif
//////////////////////////
// MIDI MESAGES
// midi.org/techspecs/
//////////////////////////
// STATUS BYTES
// 0x8n == noteOff
// 0x9n == noteOn
// 0xAn == afterTouch
// 0xBn == controlChange
//    n == Channel(0x0-0xf)
//////////////////////////
//DATA BYTE 1
// note# == (0-127)
// or
// control# == (0-119)
//////////////////////////
// DATA BYTE 2
// velocity == (0-127)
// or
// controlVal == (0-127)
///////////////////////////////////////////////////////////////////////////////
// USB-MIDI Event Packets
// usb.org - Universal Serial Bus Device Class Definition for MIDI Devices 1.0
///////////////////////////////////////////////////////////////////////////////
//+-------------+-------------+-------------+-------------+
//|   Byte 0    |   Byte 1    |   Byte 2    |   Byte 3    |
//+------+------+-------------+-------------+-------------+
//|Cable | Code |             |             |             |
//|Number|Index |   MIDI_0    |   MIDI_1    |   MIDI_2    |
//|      |Number|             |             |             |
//|(4bit)|(4bit)|   (8bit)    |   (8bit)    |   (8bit)    |
//+------+------+-------------+-------------+-------------+
// CN == 0x0-0xf
//+-----+-----------+-------------------------------------------------------------------
//| CIN |MIDI_x size|Description
//+-----+-----------+-------------------------------------------------------------------
//| 0x0 | 1, 2 or 3 |Miscellaneous function codes. Reserved for future extensions.
//| 0x1 | 1, 2 or 3 |Cable events. Reserved for future expansion.
//| 0x2 |     2     |Two-byte System Common messages like MTC, SongSelect, etc.
//| 0x3 |     3     |Three-byte System Common messages like SPP, etc.
//| 0x4 |     3     |SysEx starts or continues
//| 0x5 |     1     |Single-byte System Common Message or SysEx ends with following single byte.
//| 0x6 |     2     |SysEx ends with following two bytes.
//| 0x7 |     3     |SysEx ends with following three bytes.
//| 0x8 |     3     |Note-off
//| 0x9 |     3     |Note-on
//| 0xA |     3     |Poly-KeyPress
//| 0xB |     3     |Control Change
//| 0xC |     2     |Program Change
//| 0xD |     2     |Channel Pressure
//| 0xE |     3     |PitchBend Change
//| 0xF |     1     |Single Byte
//+-----+-----------+-------------------------------------------------------------------




/* SysEx data size counter
unsigned int UHS_NI UHS_MIDI::countSysExDataSize(uint8_t *dataptr) {
        unsigned int c = 1;

        if(*dataptr != 0xf0) { //not SysEx
                return 0;
        }

        //Search terminator(0xf7)
        while(*dataptr != 0xf7) {
                dataptr++;
                c++;

                //Limiter (upto 256 bytes)
                if(c > 256) {
                        c = 0;
                        break;
                }
        }
        return c;
}

 Send SysEx message to MIDI device
uint8_t UHS_NI UHS_MIDI::SendSysEx(uint8_t *dataptr, unsigned int datasize, byte nCable) {
        byte buf[4];
        uint8_t rc;
        unsigned int n = datasize;

        //Byte 0
        buf[0] = (nCable << 4) | 0x4; //x4 SysEx starts or continues

        while(n > 0) {
                switch(n) {
                        case 1:
                                buf[0] = (nCable << 4) | 0x5; //x5 SysEx ends with following single byte.
                                buf[1] = *(dataptr++);
                                buf[2] = 0x00;
                                buf[3] = 0x00;
                                n = n - 1;
                                break;
                        case 2:
                                buf[0] = (nCable << 4) | 0x6; //x6 SysEx ends with following two bytes.
                                buf[1] = *(dataptr++);
                                buf[2] = *(dataptr++);
                                buf[3] = 0x00;
                                n = n - 2;
                                break;
                        case 3:
                                buf[0] = (nCable << 4) | 0x7; //x7 SysEx ends with following three bytes.
                        default:
                                buf[1] = *(dataptr++);
                                buf[2] = *(dataptr++);
                                buf[3] = *(dataptr++);
                                n = n - 3;
                                break;
                }
                rc = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, 4, buf);
                if(rc != 0)
                        break;
        }
        return (rc);
}
 */
