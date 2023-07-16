#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

void performUpKeyAction( unsigned short keyCode )
{
    // Create a keyboard event for the up arrow key press
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, keyCode, true);
    
    // Post the key press event
    CGEventPost(kCGSessionEventTap, event);
    
    // Release the event
    CFRelease(event);
}

int main()
{
    performUpKeyAction( kVK_ANSI_C );  // Call the function to perform the up key action

    return 0;
}

/*
cd "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/" && g++ keyPressTest.cpp -o keyPressTest -framework CoreFoundation -framework CoreGraphics -framework ApplicationServices && "/Users/wanda/Desktop/Eason_C++_學習/c++code/project/tetrisA/"keyPressTest


*/