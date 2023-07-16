#include <iostream>
#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h>


int get_pixel_color(int x, int y){
    CGDirectDisplayID displayId = kCGDirectMainDisplay;
    CGImageRef image = CGDisplayCreateImageForRect(displayId, CGRectMake(x, y, 1, 1));
    CGDataProviderRef provider = CGImageGetDataProvider(image);
    CFDataRef data = CGDataProviderCopyData(provider);
    const UInt8* pixelData = CFDataGetBytePtr(data);

    // Color components
    int red = pixelData[0];
    int green = pixelData[1];
    int blue = pixelData[2];
    int alpha = pixelData[3];
    int colorCode = (red << 16) | (green << 8) | blue;

    // Release resources
    CFRelease(data);
    CGImageRelease(image);
    //printf("%d " , colorCode);
    return colorCode;
}

int not_main(int x, int y) {
    //std::cout << "yeet\n";
    CGDirectDisplayID displayId = kCGDirectMainDisplay;
    CGImageRef image = CGDisplayCreateImageForRect(displayId, CGRectMake(x, y, 1, 1));
    CGDataProviderRef provider = CGImageGetDataProvider(image);
    CFDataRef data = CGDataProviderCopyData(provider);
    const UInt8* pixelData = CFDataGetBytePtr(data);

    // Color components
    int red = pixelData[0];
    int green = pixelData[1];
    int blue = pixelData[2];
    int alpha = pixelData[3];
    int colorCode = (red << 16) | (green << 8) | blue;

    std::cout << "Color code at (" << x << ", " << y << "): color code: " 
    << colorCode << "\n";

    // Release resources
    CFRelease(data);
    CGImageRelease(image);

    return 0;
}

int main(){
    std::cout << "yeet\n";
    usleep(1000000);
    std::cout << get_pixel_color(198, 627) << "\n";
}

//g++ -o rgbtest rgbtest.cpp -framework CoreFoundation -framework CoreGraphics
/*
cd "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/" && g++ rgbtest.cpp -o rgbtest -framework CoreFoundation -framework CoreGraphics && "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/"rgbtest
Color code at (377, 412): color code: 13180981 J 2
Color code at (377, 411): color code: 13180981 
Color code at (377, 436): color code: 83442 L 3
Color code at (377, 468): color code: 2293993 Z 7
Color code at (377, 482): color code: 0 
Color code at (377, 551): color code: 48155 S 5
Color code at (400, 575): color code: 8978623 T 6
Color code at (517, 552): color code: 14326272 I 1
Color code at (470, 370): color code: 40683 O 4
14326272, 13180981, 83442, 40683, 48155, 8978623, 2293993

218 645 purple

*/