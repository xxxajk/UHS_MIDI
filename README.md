# USBH_MIDI v0.1.0


USB-MIDI class driver for Arduino [USB Host Shield 3.0 Library][UHS3]
Based on original code for:
USB-MIDI class driver for Arduino [USB Host Shield 2.0 Library][UHS2]

You can convert USB MIDI keyboard  to legacy serial MIDI.

Please check [device list][wiki]

## How to use

Please put into a USBH_MIDI directory to your Arduino libraries directory.

### for single device
> File->Examples->USBH_MIDI->USB_MIDI_converter

### for multiple device (with USB hub)
> File->Examples->USBH_MIDI->USB_MIDI_converter_multi

## API

### uint8_t RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr);
Receive raw USB-MIDI Event Packets (4 bytes)
return value is 0:Success, non-zero:Error(MAX3421E HRSLT)

### uint8_t RecvData(uint8_t *outBuf);
Receive MIDI messages (3 bytes)
return value is MIDI message length(0-3)

### uint8_t SendData(uint8_t *dataptr, byte nCable=0);
Send MIDI message. You can set CableNumber(default=0).
return value is 0:Success, non-zero:Error(MAX3421E HRSLT)

### uint8_t SendSysEx(uint8_t *dataptr, unsigned int datasize, byte nCable=0);
Send SysEx MIDI message. You can set CableNumber(default=0).
return value is 0:Success, non-zero:Error(MAX3421E HRSLT)
note: You must set first byte:0xf0 and last byte:0xf7

## ChangeLog
2015.03.13 xxxajk
* Initial pull, and start of port to version 3 API for UHS USB

2014.07.06
* Merge IOP_ArduinoMIDI branch into master
* Change class name to USBH_MIDI
* Rename the function RcvData to RecvData (Old name is still available)
* Fix examples for Arduino MIDI Library 4.2 compatibility
* Add SendSysEx()
* Add new example (eVY1_sample)

2014.03.23
* Fix examples for Arduino MIDI Library 4.0 compatibility and Leonardo

2013.12.20
* Fix multiple MIDI message problem.
* Add new example (USBH_MIDI_dump)

2013.11.05
* Removed all unnecessary includes. (latest UHS2 compatibility)
* Rename all example extensions to .ino

2013.08.28
* Fix MIDI Channel issue.

2013.08.18
* RcvData() Return type is changed to uint8_t.
* Fix examples.

2012.06.22
* Support MIDI out and loosen device check

2012.04.21
* First release


## License

Copyright &copy; 2012-2014 Yuuichi Akagawa

Licensed under the [GNU General Public License v2.0][GPL2]

[GPL2]: http://www.gnu.org/licenses/gpl2.html
[wiki]: https://github.com/YuuichiAkagawa/USBH_MIDI/wiki
[UHS2]: https://github.com/felis/USB_Host_Shield_2.0
[UHS3]: https://github.com/felis/UHS30
