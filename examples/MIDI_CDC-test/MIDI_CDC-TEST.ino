//#define ENABLE_UHS_DEBUGGING 1
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
#define USB_HOST_SHIELD_USE_ISR 1


//#define LOAD_UHS_KINETIS_FS_HOST
#define LOAD_USB_HOST_SHIELD



#define LOAD_USB_MIDI
#define LOAD_USB_HOST_SYSTEM


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif


#if defined(__arm__)
#include <dyn_SWI.h>
#include <SWI_INLINE.h>
#endif

#include <stdio.h>
#include <Wire.h>
#include <SPI.h>
#ifdef LOAD_UHS_KINETIS_FS_HOST
#define USB_HOST_klass UHS_KINETIS_FS_HOST
#define USB_HOST_SERIAL Serial1
#include <UHS_host.h>
#include <UHS_KINETIS_FS_HOST.h>
#endif
#ifdef LOAD_USB_HOST_SHIELD
#include <UHS_host.h>
#define USB_HOST_klass MAX3421E_HOST
#include <USB_HOST_SHIELD.h>
#endif
#include <UHS_HUB.h>
#include <UHS_MIDI.h>




#if defined(CORE_TEENSY) && defined(__arm__)

extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for(j = 0; j < len; j++) {
                        if(fd == 1)
                                USB_HOST_SERIAL.write(*ptr++);
                        else if(fd == 2)
                                USB_HOST_SERIAL.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if(len > 0 && fd == 0) {
                        while(!USB_HOST_SERIAL.available());
                        *ptr = USB_HOST_SERIAL.read();
                        return 1;
                }
                return 0;
        }

#include <sys/stat.h>

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }
}

// Else we are using CMSIS DAP
#endif // TEENSY_CORE


// Extend the MIDI driver class so that we can do the work when it polls.
class UHS_MIDI_X : public  UHS_MIDI {

public:
        uint8_t packets;
        volatile int16_t num_down;


        UHS_MIDI_X(UHS_USB_HOST_BASE *p): UHS_MIDI(p) {
                pinMode(LED_BUILTIN, OUTPUT);
                num_down = 0;
        };

        void UHS_NI CHECK(void) {
                digitalWrite(LED_BUILTIN, HIGH);
                uint8_t rcode;
                packets = 0;
                pUsb->DisablePoll();
                rcode = Read();
                if(!rcode) {
                        if(recvBuf[0] == 0xF0) {
                                // SYSEXEC packet, skip this for now.
                        } else {
                                for(int i=0; i<rcvd; i+=4) {
                                        if(!msgsztbl[recvBuf[i]]) break;
                                        packets++;
                                }
                        }
                }
                pUsb->EnablePoll();
                digitalWrite(LED_BUILTIN, LOW);

        };
};


#define MAKE_USB_HOST_PTR(NAME) USB_HOST_klass *NAME
#define MAKE_USB_HUB_PTR(NAME) UHS_USBHub *NAME
#define MAKE_USB_MIDI_X_PTR(NAME) UHS_MIDI_X *NAME

// I use new in setup() just to show that you can do it that way too.
// Older versions can't do it.
MAKE_USB_HOST_PTR(HOST_PORT_0);
MAKE_USB_HUB_PTR(HUB_0);
MAKE_USB_MIDI_X_PTR(MIDI_0);

volatile uint8_t usbstate;
volatile uint8_t laststate;


#if defined(__AVR__)
// AVR printf hack. Yes malloc and printf is all ISR safe when using the UHS library!
// For ARM with CMSIS-DAP, this is typically built into the programming port.
// for ARM without CMSIS-DAP, this is done a little differently.
static FILE mystdout;
static int my_putc(char c, FILE *t) {
        USB_HOST_SERIAL.write(c);
        return 0;
}
#endif

void setup() {
#if defined(SWI_IRQ_NUM)
        Init_dyn_SWI();
#endif
#if defined(__AVR__)
        mystdout.put = my_putc;
        mystdout.get = NULL;
        mystdout.flags = _FDEV_SETUP_WRITE;
        mystdout.udata = 0;
        stdout = &mystdout;
#endif

        delay(2000);
        USB_HOST_SERIAL.begin(115200);
        printf("\r\n\r\nhello world\r\n");
        fflush(stdout);
        USB_HOST_SERIAL.flush();
        HOST_PORT_0 = new USB_HOST_klass();
        HUB_0 = new UHS_USBHub(HOST_PORT_0 );
        MIDI_0 = new UHS_MIDI_X(HOST_PORT_0 );
        while (HOST_PORT_0->Init());
}

void loop()
{
        usbstate = HOST_PORT_0->getUsbTaskState();
        if(usbstate != laststate) {
                printf("FSM state: 0x%2.2X\r\n", usbstate);
                USB_HOST_SERIAL.flush();
                laststate = usbstate;
        }
        if(MIDI_0->Polling()) {
                bool more;
                do {
                        more = false;
                        MIDI_0->CHECK();
                        if(MIDI_0->packets) {
                                more = true;
                                for(int i=0; i< MIDI_0->packets; i++) {
                                        if(MIDI_0->recvBuf[i*4] == 0x08) {
                                                // up
                                                MIDI_0->num_down--;
                                        } else if(MIDI_0->recvBuf[i*4] == 0x09) {
                                                // down    
                                                MIDI_0->num_down++;    
                                        //} else {
                                        //        printf("%0.2x ",  MIDI_0->recvBuf[i*4]);
                                        }
                                }
                                printf("Number of keys down: %i\r\n", MIDI_0->num_down);
                                MIDI_0->packets = 0;
                        }
                } while(more);
                
        }
//        if(MIDI_0->wacked) {
//        if(MIDI_0->Polling()) {
//                delay(1000);
//                noInterrupts();
//                num_down = MIDI_0->num_down;
//                MIDI_0->wacked = false;
//                interrupts();
//                printf("MIDI_0: %i\r\n", num_down);
//                USB_HOST_SERIAL.flush();
//        }
        
}

