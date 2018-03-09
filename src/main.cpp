// TO DO
// Keep track of the window ID instead of the count number in the bitmap
// output bitmap<xid>.bmp instead of bitmap%d.bmp

#include <iostream>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/extensions/Xcomposite.h>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <glog/logging.h>                                                       // Google log API
#include <string>
#include <sstream>                                                              // Needed for std patch
#include <X11/Xatom.h>                                                          // Access wm atoms
#include <fstream>

using namespace std;

namespace patch                                                                 // Fixes std in g++
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

int NotGUIWindow(Display* display, XErrorEvent* e) {                            // Error handler for input-only windows
  //ErrorHandler Ignore inside
  return 0;
}

bool is_valid_window(Display* display, Window window){                          // Function to determine if a window is top-level window
  Atom actual_type;
  int actual_format;
  Atom a = XInternAtom(display, "_NET_WM_WINDOW_TYPE" , true);
  Atom b = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL" , true);
  Atom c = XInternAtom(display, "_NET_WM_STATE_HIDDEN", true);
  unsigned long i, num_items, bytes_after;
  Atom *atoms;

  atoms=NULL;
  // Get window properties so compare the atoms
  XGetWindowProperty(display, window, a, 0, 1024, False, XA_ATOM, &actual_type, &actual_format, &num_items, &bytes_after, (unsigned char**)&atoms);
  XWindowAttributes attr;
    try{
        XSetErrorHandler(NotGUIWindow);
        Status s = XGetWindowAttributes (display, window, &attr);

        if (s == 0)
            printf ("Fail to get window attributes!\n");
    }catch(...) {
        printf("Error! Not a window\n");
    }
        
    for(i=0; i<num_items; ++i) {
        if(/*atoms[i]==b && */atoms[i]!=c /*&& attr.map_state == 2*/) {
          XFree(atoms);
          return true;
        }
    }
    XFree(atoms);
    return false;

}

void saveXImageToBitmap(XImage *pImage, Window xid);
void flipVertically(char *pixels_buffer, const unsigned int width, const unsigned int height, const int bytes_per_pixel)
{
    const unsigned int rows = height / 2; // Iterate only half the buffer to get a full flip
    const unsigned int row_stride = width * bytes_per_pixel;
    char* temp_row = (char*)malloc(row_stride);
    //new char* temp_row;

    int source_offset, target_offset;

    for (int rowIndex = 0; rowIndex < rows; rowIndex++)
    {
        source_offset = rowIndex * row_stride;
        target_offset = (height - rowIndex - 1) * row_stride;

        memcpy(temp_row, pixels_buffer + source_offset, row_stride);
        memcpy(pixels_buffer + source_offset, pixels_buffer + target_offset, row_stride);
        memcpy(pixels_buffer + target_offset, temp_row, row_stride);
    }

    free(temp_row);
    temp_row = NULL;
}

int main()
{    
    //usleep(4000000);
    ofstream myfile;
    myfile.open ("window-data.csv");
    Display *display;
    int s;
    Window xid;
    Window root_return;
    Window parent_return;
    Window *children_return;
    char *window_name;
    unsigned int nchildren_return;
    std::string str;

    display = XOpenDisplay(0);
    s = DefaultScreen(display);
    int count = 0;
        
    int event_base_return;
    int error_base_return;
    if (XCompositeQueryExtension (display, &event_base_return, &error_base_return))
        printf ("COMPOSITE IS ENABLED!\n");
        
    XQueryTree(display, XDefaultRootWindow(display), &root_return, &parent_return, &children_return, &nchildren_return);
    printf("Total number of xWindows is: %u\n", nchildren_return);    
    str.append (patch::to_string(nchildren_return));
    str.append ("\n");
    for(int i = 0; i < nchildren_return; i++)
    {   
        
        XFetchName(display, *(children_return+i), &window_name);
        
        xid = *(children_return+i);

        
        XCompositeRedirectWindow (display, xid, CompositeRedirectAutomatic);
        
        XWindowAttributes attr;
        try{
            XSetErrorHandler(NotGUIWindow);
            Status s = XGetWindowAttributes (display, xid, &attr);

            if (s == 0)
                printf ("Fail to get window attributes!\n");
        }catch(...) {
            printf("Error! Not a window\n");
        }
        int width = attr.width;
        int height = attr.height;
        int depth = attr.depth;

        
        std::stringstream ss;
        XImage *img = XGetImage(display,xid,0,0,width,height,XAllPlanes(),ZPixmap);

        if (img != NULL)
        {
            printf("\nTitle = %s\n", window_name);
            ss << std::hex << xid;
            printf("0x%lx\n", *(children_return+i));
            printf("w: %d h: %d d: %d\n", width, height, depth);
            saveXImageToBitmap(img, xid);
            //save image here

            str.append(patch::to_string(window_name) + ",");
            str.append(ss.str() + ",");
            str.append(patch::to_string(width) + "," + patch::to_string(height) + ",bitmap");
            str.append(ss.str());
            count++;
            str.append("\n");
        }
        
        XCompositeUnredirectWindow (display, xid, CompositeRedirectAutomatic);
    
        XFree(window_name);

    }
    myfile << str;
    myfile.close();
    XCloseDisplay(display);
    return 0;
}

#pragma pack (1)
typedef struct BITMAPFILEHEADER 
{
short    bfType;
int    bfSize;
short    bfReserved1;
short    bfReserved2;
int   bfOffBits;
};

typedef struct BITMAPINFOHEADER
{
int  biSize;
int   biWidth;
int   biHeight;
short   biPlanes;
short   biBitCount;
int  biCompression;
int  biSizeImage;
int   biXPelsPerMeter;
int   biYPelsPerMeter;
int  biClrUsed;
int  biClrImportant;
};

void saveXImageToBitmap(XImage *pImage, Window xid)
{
BITMAPFILEHEADER bmpFileHeader;
BITMAPINFOHEADER bmpInfoHeader;
FILE *fp;

int dummy;
char filePath[255];
memset(&bmpFileHeader, 0, sizeof(BITMAPFILEHEADER));
memset(&bmpInfoHeader, 0, sizeof(BITMAPINFOHEADER));
bmpFileHeader.bfType = 0x4D42;
bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
bmpFileHeader.bfReserved1 = 0;
bmpFileHeader.bfReserved2 = 0;
int biBitCount =32;
int dwBmpSize = ((pImage->width * biBitCount + 31) / 32) * 4 * pImage->height;
printf("size of short:%d\r\n",(int)sizeof(short));
printf("size of int:%d\r\n",(int)sizeof(int));
printf("size of long:%d\r\n",(int)sizeof(long));
printf("dwBmpSize:%d\r\n",(int)dwBmpSize);
printf("BITMAPFILEHEADER:%d\r\n",(int)sizeof(BITMAPFILEHEADER));
printf("BITMAPINFOHEADER:%d\r\n",(int)sizeof(BITMAPINFOHEADER));
bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +  dwBmpSize;

bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
bmpInfoHeader.biWidth = pImage->width;
bmpInfoHeader.biHeight = pImage->height;
bmpInfoHeader.biPlanes = 1;
bmpInfoHeader.biBitCount = biBitCount;
bmpInfoHeader.biSizeImage = 0;
bmpInfoHeader.biCompression = 0;
bmpInfoHeader.biXPelsPerMeter = 0;
bmpInfoHeader.biYPelsPerMeter = 0;
bmpInfoHeader.biClrUsed = 0;
bmpInfoHeader.biClrImportant = 0;

sprintf(filePath, "bitmap%lx.bmp", xid);
fp = fopen(filePath,"wb");

if(fp == NULL)
    return;

flipVertically(pImage->data, pImage->width, pImage->height, 4);
//flipVertically(&(reinterpret_cast<unsigned char&>(pImage->data)), pImage->width, pImage->height, pImage->depth);
fwrite(&bmpFileHeader, sizeof(bmpFileHeader), 1, fp);
fwrite(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, fp);
fwrite(pImage->data, dwBmpSize, 1, fp);
fclose(fp);
}
