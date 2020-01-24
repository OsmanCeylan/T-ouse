#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <fstream>
#include <vector>

#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libusb.h>
#include <linux/uinput.h>

void helloScreen(void);
bool checkAdminRights(void);
int findUsbDevice(struct libusbInit &, struct Device &);
bool handleArgs(int &, char **, struct Device &);
void displayHelpScreen(char *argv);
int initializeUsb(struct libusbInit &);
bool openDevice(struct Device &);
bool removeFromKernel(struct Device &);
bool claimInterface(struct Device &);
int setupMouse(struct FakeMouse &);
void sighandler(int );
void dummyInput(struct FakeMouse &);
void deviceLoop(struct Device &, struct FakeMouse &, struct Configurations &);
void releaseAll(struct Device &, struct FakeMouse &, struct libusbInit &);
int listALlUsbDevices();
bool readFromFile(struct Configurations &);
bool prepareCalculations(struct Configurations &);



/*------------------------
 * @Libusb things here,
 * @listdevice
 * @get count of devices
 * @hold initializer
 * @hold libusb context
 * ---------------------*/

struct libusbInit{
    libusb_device **devs;
    int faultNum;
    ssize_t count;
    libusb_context *ctx;

    libusbInit(void)
    {
        devs = NULL;
        faultNum = 0;
        count = 0;
        ctx = NULL;
    }
};


/*------------------------
 * @Tablet Device
 * @VID, PID
 * @Descriptions
 * @Device handler
 * @Interface number
 * @Endpoint
 * @--test argument
 * ---------------------*/

struct Device{
    uint16_t DevID;
    uint16_t ProID;
    libusb_device *dev;
    struct libusb_device_descriptor desc;
    libusb_device_handle *dev_handle;
    int claimInteface;
    unsigned char interfaceEndPoint;
    bool deviceDataTest;

    Device(void)
    {
        DevID = 0xFFFF;
        ProID = 0x0000;
        dev = NULL;
        dev_handle = NULL;
        claimInteface = 0;
        interfaceEndPoint = 0;
        deviceDataTest = false;             //--test option
    }
};


/*------------------------
 * @Fake mouse
 * @VID, PID
 * @Device number
 * @Device dpi
 * ---------------------*/
struct FakeMouse{
    int fd;
    uint16_t DevID;
    uint16_t ProID;
    std::string name;
    int DPI;

    FakeMouse(void)
    {
        fd = -1;
        DevID = 0xFFFF;
        ProID = 0x0000;
        name = "T-ouse";
        DPI = 900;
    }
};


/*---------------------------------------
 * @Opens and reads data from "touse.conf
 * @Tablet and mouse configurations
 * @Read more on touse.conf
 * -------------------------------------*/

struct Configurations{
    std::string filename;
    std::string tabletName;
    std::string tVenID;
    std::string tProID;
    float activeAreaW;
    float activeAreaH;
    float LPI;
    int PEN_IO_DATA, PEN_PR_DATA, PEN_BTTN1_DATA, PEN_BTTN2_DATA;
    int PEN_INSIDE_VAR, PEN_OUTSIDE_VAR, PEN_PRESS_VAR, PEN_BTTN1_VAR, PEN_BTTN2_VAR;
    int PEN_MAX_W;
    int PEN_MAX_H;

    std::string mouseName;
    std::string mVenID;
    std::string mProID;
    float DPI;

    Configurations(void)
    {
        filename = "touse.conf";

        tabletName = "none";
        tVenID = "0x0000";
        tProID = "0x0000";
        activeAreaW = 0.0f;
        activeAreaH = 0.0f;
        LPI = -1.0f;

        PEN_MAX_W = 0;
        PEN_MAX_H = 0;

        PEN_IO_DATA = 1;
        PEN_PR_DATA = 1;
        PEN_BTTN1_DATA = 1;
        PEN_BTTN2_DATA = 1;

        PEN_INSIDE_VAR = -1;
        PEN_OUTSIDE_VAR = -1;
        PEN_PRESS_VAR = -1;
        PEN_BTTN1_VAR = -1;
        PEN_BTTN2_VAR = -1;

        mouseName = "";
        mVenID = "0x0000";
        mProID = "0x0000";
        DPI = 900.0f;
    }

};

#endif // MAIN_H
