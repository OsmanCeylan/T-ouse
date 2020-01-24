## T-ouse
Using tablet as a mouse workaround
It is simple program that creates a fake mouse device with UInput and redirects tablet inputs to mouse.
Tablet inputs generates mouse inputs instead of a pixel pointer of the screen.

It may useless as if you are doing tablet related things but you can play games with tablet.
Since tablet act like mouse, games like CS:GO can be playable with tablet, as this project aiming.

If you have your tablet data sheet, you can use this program as a tablet driver for any tablet.

# Build

It supports only Linux for now. Because reading, writing and faking a device is hard in Windows. Not to mention HID stuffs.

## Linux

Program depends on libusb>1 and Uinput>any

make -f Makefile
Run program as administrator.

# Use

## Usage 
--help:     Displays help
--listall:  Lists all usb devices connected
--device:   Displays all information about given device [VenID] [ProID]
--use:      Runs program as given [VenID] [ProID] [Interface] [Endpoint]
--test:     Act similar as --use but diplays device I/O data

## Using

After finding tablet vendor and product id's, you need to determine which interface and endpoint yield input data.
After finding all interface and endpoint numbers, you should test program with all interface and related endpoints with using --test. Only one interface and related endpoint dumps data on output. 

# Configuring

Program uses touse.conf as configure file which contain tablet and fake mouse configurations.
Current configurations works on Huion H420.(and probably majority of all Huions)
For configuring other tablets, refer to configure file. There are instructions of how to do it.

## X, Y, Z calculations

If you use tablet other than Huion (H)420, you need to also find calculation formulas.
Before compiling the program;

main.cpp, In function: void deviceloop(), replace the following lines with related formulas.
  /*---------------------------------
  | @Huion general x,y,z calcualtion
  ----------------------------------*/
   x = (data[3]*255 + data[2])*2;
   y = (data[5]*255 + data[4])*2;
   z = data[7]*255 + data[6];


