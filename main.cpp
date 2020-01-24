#include "main.h"

using namespace std;

int main(int argc, char **argv)
{
    Device usbDevice;
    FakeMouse myMouse;
    libusbInit myInit;
    Configurations myConf;

    helloScreen();

    if (!checkAdminRights())                      return 1;
    if (!handleArgs(argc, argv, usbDevice))       return 1;

    initializeUsb(myInit);
    if (findUsbDevice(myInit, usbDevice) == 0)    return 1;

    if (!openDevice(usbDevice))                   {releaseAll(usbDevice, myMouse, myInit); return 1;}
    if (!removeFromKernel(usbDevice))             {releaseAll(usbDevice, myMouse, myInit); return 1;}
    if (!claimInterface(usbDevice))               {releaseAll(usbDevice, myMouse, myInit); return 1;}

    if (!readFromFile(myConf))                    {releaseAll(usbDevice, myMouse, myInit); return 1;}
    if (!setupMouse(myMouse))                     {releaseAll(usbDevice, myMouse, myInit); return 1;}

    /*------------------------------------
    | @ Initialize signals and slots
    -------------------------------------*/
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, SIG_IGN);

    dummyInput(myMouse);
    deviceLoop(usbDevice, myMouse, myConf);
    releaseAll(usbDevice, myMouse, myInit);

    return 0;
}

bool readFromFile(struct Configurations &myConf)
{
    string *line = new string;
    ifstream myfile;
    myfile.open(myConf.filename);

    if (!myfile.is_open())
    {
        cout << "Error while open file" << endl;
        return false;
    }

    bool tabletConfFound = false;
    bool mouseConfFound = false;

    while ( getline (myfile,*line) )
    {
        if (*line == "device defaultTablet:")
        {
            tabletConfFound = true;

            while (myfile >> *line)
            {
               if (*line == "Name"){
                   myfile >> *line;
                   myConf.tabletName = *line;
               }
               if (*line == "END"){
                   break;
               }
               if (*line == "VendorID"){
                   myfile >> *line;
                   myConf.tVenID = *line;
               }
               if (*line == "ProductID"){
                   myfile >> *line;
                   myConf.tProID = *line;
               }
               if (*line == "Active Area in Width"){
                   myfile >> *line;
                   myConf.activeAreaW = stof(*line);
               }
               if (*line == "Active Area in Height"){
                   myfile >> *line;
                   myConf.activeAreaH = stof(*line);
               }
               if (*line == "LPI"){
                   myfile >> *line;
                   myConf.LPI = stoi(*line);
               }
               if (*line == "PEN_IO_DATA"){
                   myfile >> *line;
                   myConf.PEN_IO_DATA = stoi(*line);
               }
               if (*line == "PEN_PR_DATA"){
                   myfile >> *line;
                   myConf.PEN_PR_DATA = stoi(*line);
               }
               if (*line == "PEN_BTTN1_DATA"){
                   myfile >> *line;
                   myConf.PEN_BTTN1_DATA = stoi(*line);
               }
               if (*line == "PEN_BTTN2_DATA"){
                   myfile >> *line;
                   myConf.PEN_BTTN2_DATA = stoi(*line);
               }
               if (*line == "PEN_INSIDE_VAR"){
                   myfile >> *line;
                   myConf.PEN_INSIDE_VAR = stoi(*line);
               }
               if (*line == "PEN_OUTSIDE_VAR"){
                   myfile >> *line;
                   myConf.PEN_OUTSIDE_VAR = stoi(*line);
               }
               if (*line == "PEN_PRESS_VAR"){
                   myfile >> *line;
                   myConf.PEN_PRESS_VAR = stoi(*line);
               }
               if (*line == "PEN_BTTN1_VAR"){
                   myfile >> *line;
                   myConf.PEN_BTTN1_VAR = stoi(*line);
               }
               if (*line == "PEN_BTTN2_VAR"){
                   myfile >> *line;
                   myConf.PEN_BTTN2_VAR = stoi(*line);
               }
            }
        }

        if (*line == "device defaultMouse:")
        {
            mouseConfFound = true;

            while (myfile >> *line)
            {
                if (*line == "Name"){
                    myfile >> *line;
                    myConf.mouseName = *line;
                }
                if (*line == "VendorID"){
                    myfile >> *line;
                    myConf.mVenID = *line;
                }
                if (*line == "ProductID"){
                    myfile >> *line;
                    myConf.mProID = *line;
                }
                if (*line == "DPI"){
                    myfile >> *line;
                    myConf.DPI = stoi(*line);
                }
                if (*line == "END"){
                    break;
                }
            }
        }

        if (*line == "ENDOFFILE")
        {
            break;
        }
    }

    myfile.close();
    if (!tabletConfFound || !mouseConfFound)
    {
        cout << "Some important setting cannot determined. Check at configuration file for whitespaces" << endl;
        return false;
    }

    myConf.PEN_MAX_W = static_cast<int>(myConf.LPI * myConf.activeAreaW);
    myConf.PEN_MAX_H = static_cast<int>(myConf.LPI * myConf.activeAreaH);

    return true;
}


static void emit(int fd, int type, int code, int val) {
    struct input_event ie;

    ie.type = static_cast<__u16>(type);
    ie.code = static_cast<__u16>(code);
    ie.value = static_cast<__s32>(val);
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

void deleteDevice(int fd) {
    if (fd > 0) {
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
    }
}

int setupMouse(struct FakeMouse &myMouse) {
    struct uinput_setup usetup;

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        cout << "failed to open device %s\n" << static_cast<string>(strerror(errno)) << endl;
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0xAAAA;
    usetup.id.product = 0xBBBB;
    strcpy(usetup.name, "Uinput");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
    sleep(1);

    myMouse.fd = fd;
    cout << "Mouse setup complate" << endl;
    return true;
}

void printdev(libusb_device *dev) {
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
        cout<<"failed to get device descriptor"<<endl;
        return;
    }

    cout<<"Number of possible configurations: "<< static_cast<int>(desc.bNumConfigurations)<<"  ";
    cout<<"Device Class: "<<static_cast<int>(desc.bDeviceClass)<<"  ";
    cout<<"VendorID: "<<desc.idVendor<<"  ";
    cout<<"ProductID: "<<desc.idProduct<<endl;
    libusb_config_descriptor *config;
    libusb_get_config_descriptor(dev, 0, &config);
    cout<<"Interfaces: "<<static_cast<int>(config->bNumInterfaces)<<"\n";
    const libusb_interface *inter;
    const libusb_interface_descriptor *interdesc;
    const libusb_endpoint_descriptor *epdesc;

    for(int i=0; i<static_cast<int>(config->bNumInterfaces); i++) {
        inter = &config->interface[i];
        cout<<"Number of alternate settings: "<<inter->num_altsetting<<" | ";
        for(int j=0; j<inter->num_altsetting; j++) {
            interdesc = &inter->altsetting[j];
            cout<<"Interface Number: "<<static_cast<int>(interdesc->bInterfaceNumber)<<" | ";
            cout<<"Number of endpoints: "<<static_cast<int>(interdesc->bNumEndpoints)<<" | ";
            for(int k=0; k<static_cast<int>(interdesc->bNumEndpoints); k++) {
                epdesc = &interdesc->endpoint[k];
                cout<<"Descriptor Type: "<<static_cast<int>(epdesc->bDescriptorType)<<" | ";
                cout<<"EP Address: "<<static_cast<int>(epdesc->bEndpointAddress)<<" | ";
            }
        }
        cout << endl;
    }

    cout<<endl<<endl;
    libusb_free_config_descriptor(config);
}

void sighandler(int i) {
    (void)i;
    close(0);
}

void pointerMove(int fd, int x, int y) {
    emit(fd, EV_REL, REL_X, x);
    emit(fd, EV_REL, REL_Y, y);
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

void pointerClickPress(int fd) {
    emit(fd, EV_KEY, BTN_MOUSE, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

void pointerClickRelease(int fd) {
    emit(fd, EV_KEY, BTN_MOUSE, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
}


void helloScreen()
{

    const struct libusb_version *libusbversion = NULL;
    libusbversion = libusb_get_version();

    cout << " ---------- T-ouse: 0.0.1 ---------------" << endl;

    cout << "Using: libusb-" ;
    cout << libusbversion->major << libusbversion->minor << libusbversion->micro << endl;

    cout << "Using: UInput-";
    cout << UI_GET_VERSION << endl;

}

bool checkAdminRights()
{
    if (getuid())
    {
        cout << "Are you root?!" << endl;
        return false;
    }
    else
        return true;
}

/*-------------------------------------
|  @Success returns 1
|  @else    refer to libusb manuals
--------------------------------------*/

int initializeUsb(struct libusbInit &init)
{
    init.faultNum = libusb_init(&init.ctx);
    if (init.faultNum < 0)
    {
        cout << "Linusb initialize failed" << endl;
        return init.faultNum;
    }
    else
        return 1;
}

void str_to_uint16(char *hex, uint16_t *res)
{
    int total = 0, val = 0;
    int power = 0;

    for (size_t i=strlen(hex)-1; i>=2; i--)
    {

        if(hex[i]>='0' && hex[i]<='9')
        {
            val = hex[i] - 48;
        }

        else if(hex[i]>='a' && hex[i]<='f')
        {
            val = hex[i] - 97 + 10;
        }

        else if(hex[i]>='A' && hex[i]<='F')
        {
            val = hex[i] - 65 + 10;
        }
        total += static_cast<int>(val*pow(16,power));
        power++;
    }

    *res = static_cast<uint16_t>(total);
}

void displayHelpScreen(char *argv)
{
    cout << "Usage: " << argv << " ([OPTION]) [devID] [venID] [interface] [endPoint]" << endl;
    cout << "Exapmle: " << argv  << " --use 0x1234 " << "0xABCD 0 129   (x must be small)" << endl;
    cout << "Options: \n\n" << "\t--use     : Main usage,\n" <<
                               "\t--test    : Main usage with data[8] feedback\n" <<
                               "\t--version : Displays version text\n" <<
                               "\t--help    : Displays this\n" <<
                               "\t--listall : List all devices\n" <<
                               "\t--device  : Displays all attributes of given usb device,\n"
                                              "\t\t\tuse with [devID] [venID]" << endl;
}


bool handleArgs(int &argc, char **argv, struct Device &usbDevice)
{
    if (argc == 1)
    {
        displayHelpScreen(argv[0]);
        return false;
    }

    if (argc == 2)
    {
        if (strcmp ("--version",argv[1])==0)
        {
            cout << "T-ouse Ver: 0.0.1-Alpha" << endl;
            return false;
        }
        else if (strcmp ("--help",argv[1])==0)
        {
            displayHelpScreen(argv[0]);
            return false;
        }
        else if (strcmp ("--listall",argv[1])==0)
        {
            listALlUsbDevices();
            return false;
        }
        else
        {
            displayHelpScreen(argv[0]);
            return false;
        }
    }


    if (argc == 3 || argc == 5)
    {
        displayHelpScreen(argv[0]);
        return false;
    }

    if (argc == 4)
    {
        if (strcmp ("--device",argv[1])==0)
        {
            if (argv[2][0] == '0' && argv[2][1] == 'x' && argv[3][0] == '0' && argv[3][1] == 'x'
                                  && strlen(argv[2]) == 6 && strlen(argv[3]) == 6){

                libusbInit myInit;
                initializeUsb(myInit);

                str_to_uint16(argv[2], &usbDevice.DevID);
                str_to_uint16(argv[3], &usbDevice.ProID);

                findUsbDevice(myInit, usbDevice);
                libusb_exit(myInit.ctx);
                return false;
            }
            else
            {
                displayHelpScreen(argv[0]);
                return false;
            }
        }
        else
        {
            displayHelpScreen(argv[0]);
            return false;
        }
    }

    if (argc == 6)
    {
        if (strcmp ("--use",argv[1])==0)
        {
            if (argv[2][0] == '0' && argv[2][1] == 'x' && argv[3][0] == '0' && argv[3][1] == 'x'
                              && strlen(argv[2]) == 6 && strlen(argv[3]) == 6){
                str_to_uint16(argv[2], &usbDevice.DevID);
                str_to_uint16(argv[3], &usbDevice.ProID);
                usbDevice.claimInteface = stoi(argv[4]);
                usbDevice.interfaceEndPoint = static_cast<unsigned char>(stoi(argv[5]));
                return true;
            }
            else
            {
                displayHelpScreen(argv[0]);
                return false;
            }

        }

        else if (strcmp ("--test",argv[1])==0)
        {
            if (argv[2][0] == '0' && argv[2][1] == 'x' && argv[3][0] == '0' && argv[3][1] == 'x'
                              && strlen(argv[2]) == 6 && strlen(argv[3]) == 6){
                str_to_uint16(argv[2], &usbDevice.DevID);
                str_to_uint16(argv[3], &usbDevice.ProID);
                usbDevice.claimInteface = stoi(argv[4]);
                usbDevice.deviceDataTest = true;
                usbDevice.interfaceEndPoint = static_cast<unsigned char>(stoi(argv[5]));
                return true;
            }
            else
            {
                displayHelpScreen(argv[0]);
                return false;
            }

        }

        else
        {
            displayHelpScreen(argv[0]);
            return false;
        }
    }

    displayHelpScreen(argv[0]);
    return false;
}


int listALlUsbDevices()
{
    libusb_device **devs = nullptr;
    libusb_device *dev;
    int i = 0, j = 0;
    uint8_t path[8];

    libusb_init(NULL);
    ssize_t cnt = 0;
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0){
       cout << "There is a problem when scan for devices" << endl;
       libusb_exit(NULL);
       return 0;
    }

    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            cout << "There is a problem when scan for devices" << endl;
            libusb_exit(NULL);
            return 0;
        }

       printf("%04x:%04x (bus %d, device %d)",
           desc.idVendor, desc.idProduct,
           libusb_get_bus_number(dev), libusb_get_device_address(dev));

       r = libusb_get_port_numbers(dev, path, sizeof(path));
       if (r > 0) {
           printf(" path: %d", path[0]);
           for (j = 1; j < r; j++)
               printf(".%d", path[j]);
        }
        printf("\n");
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    return 0;
}

int findUsbDevice(struct libusbInit &init, Device &usbDevice)
{
    libusb_device **devs;
    libusb_device *dev;
    uint8_t path[8];
    int i=0;

    init.count = libusb_get_device_list(init.ctx, &devs);
    if (init.count < 0){
        libusb_exit(NULL);
        cout << "There was a error during he usb device scan" << endl;
        libusb_free_device_list(devs, 1);
        return static_cast<int>(init.count);
    }

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            cout << "There was a error during he usb device scan" << endl;
            libusb_free_device_list(devs, 1);
            return 0;
        }

        r = libusb_get_port_numbers(dev, path, sizeof(path));

        if (desc.idVendor == usbDevice.DevID && desc.idProduct == usbDevice.ProID)
        {
            cout << "Found Device: " << usbDevice.DevID << "," << usbDevice.ProID << endl;
            usbDevice.dev = dev;
            usbDevice.desc = desc;
            printdev(usbDevice.dev);
            libusb_free_device_list(devs, 1);
            return 1;
        }

    }
    cout << "Could not find the device! Maybe list all usb devices first?" << endl;
    libusb_free_device_list(devs, 1);
    return 0;
}

bool openDevice(struct Device &usbDevice)
{
    libusb_open(usbDevice.dev, &usbDevice.dev_handle);
    if(usbDevice.dev_handle == NULL)
    {
        cout<<"Cannot open device, maybe some process using it?"<<endl;
        return false;
    }
    else
    {
        cout<<"Device Opened"<<endl;
        return true;
    }
}

bool removeFromKernel(struct Device &usbDevice)
{
    if(libusb_kernel_driver_active(usbDevice.dev_handle, 0) == 1)
    {
        if(libusb_detach_kernel_driver(usbDevice.dev_handle, 0) == 0)
        {
            cout<<"Kernel Driver Detached!"<<endl;
            return true;
        }
        else
        {
            cout << "Kernel Driver Cannot Detached!" << endl;
            return false;
        }
    }
    return true;
}

bool claimInterface(struct Device &usbDevice)
{
    int r = libusb_claim_interface(usbDevice.dev_handle, usbDevice.claimInteface);
    if(r < 0)
    {
        cout<<"Cannot Claim Interface"<<endl;
        return false;
    }
    cout<<"Claimed Interface"<<endl;
    return true;
}

void dummyInput(struct FakeMouse &myMouse)
{
    cout << "Dummy input..." << endl;
    pointerMove(myMouse.fd, -111, -111);
}

void deviceLoop(struct Device &usbDevice, struct FakeMouse &myMouse, struct Configurations &myConf)
{
    int actual_length;

    int oldX=0, oldY=0, r = 0;
    int Xx = 0, Yy = 0, Zz = 0;
    bool deatched = false;
    bool clicked = false;
    bool loop = true;
    long long totalAction=0;
    unsigned char *data = new unsigned char[8];

    float x=0, y=0, z=0;

    while(loop){
        r = libusb_interrupt_transfer(usbDevice.dev_handle, (usbDevice.interfaceEndPoint | LIBUSB_ENDPOINT_IN), data, sizeof(data), &actual_length, 0);
        if(r == 0){

            /*---------------------------------
            | @Huion general x,y,z calcualtion
            ----------------------------------*/
            x = (data[3]*255 + data[2])*2;
            y = (data[5]*255 + data[4])*2;
            z = data[7]*255 + data[6];

            /*--------------------------------------
            | @DPI and acceletaion settings should
            |                             done here
            ---------------------------------------*/

            Xx = static_cast<int>(x * myConf.DPI/myConf.LPI);
            Yy = static_cast<int>(y * myConf.DPI/myConf.LPI);
            Zz = static_cast<int>(z * myConf.DPI/myConf.LPI);

            if (static_cast<int>(data[myConf.PEN_IO_DATA]) == myConf.PEN_OUTSIDE_VAR)
                deatched = true;

            if (deatched && static_cast<int>(data[myConf.PEN_IO_DATA])!=myConf.PEN_OUTSIDE_VAR){
                oldX = Xx;
                oldY = Yy;
                deatched=false;
            }

            pointerMove(myMouse.fd, Xx-oldX, Yy-oldY);
            if (!clicked && static_cast<int>(data[myConf.PEN_PR_DATA]) == myConf.PEN_PRESS_VAR){
                pointerClickPress(myMouse.fd);
                clicked = true;
            }

            if (clicked && static_cast<int>(data[myConf.PEN_PR_DATA])!=myConf.PEN_PRESS_VAR){
                pointerClickRelease(myMouse.fd);
                clicked = false;
            }

            if (!clicked && static_cast<int>(data[myConf.PEN_BTTN1_DATA]) == myConf.PEN_BTTN1_VAR){
                pointerClickPress(myMouse.fd);
                clicked = true;
            }

            if (clicked && static_cast<int>(data[myConf.PEN_BTTN2_DATA])!=myConf.PEN_BTTN2_VAR){
                pointerClickRelease(myMouse.fd);
                clicked = false;
            }


            if (usbDevice.deviceDataTest)
                cout << "Data: " << static_cast<int>(data[0]) << " " << static_cast<int>(data[1]) << " " << static_cast<int>(data[2]) <<
                         " " << static_cast<int>(data[3]) << " " << static_cast<int>(data[4]) << " " << static_cast<int>(data[5])<< " "  <<
                        static_cast<int>(data[6]) << " " << static_cast<int>(data[7]) << " | " << Xx << " " << Yy << " " << Zz << " " <<
                        "Rel: " << Xx-oldX << "-" << Yy-oldY << endl;
            totalAction++;
            oldX=Xx;
            oldY=Yy;
        }
    }
    delete[] data;
}

void releaseAll(struct Device &usbDevice, struct FakeMouse &myMouse, struct libusbInit &init)
{
    deleteDevice(myMouse.fd);
    int r = libusb_release_interface(usbDevice.dev_handle, usbDevice.claimInteface);
    if(r!=0)
        cout<<"Cannot Release Interface"<<endl;
    cout<<"Released Interface"<<endl;
    libusb_close(usbDevice.dev_handle);
    libusb_exit(init.ctx);
}

