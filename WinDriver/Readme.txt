

                                 LOWCDC.SYS


    This is the Readme file about patch drivers for using low-speed CDC
    (Communication Device Class) protocol on Windows. For more information
    please visit
        http://www.recursion.jp/avrcdc/

    The lowcdc.sys are developed by Osamu Tamura.


SUMMARY
=======
    The "avrcdc.inf" and "lowcdc.inf" are INF files to load Windows' pre-
    installed driver "usbser.sys", which performs virtual COM ports over the
    CDC protocol.

    The "lowcdc.sys" performs the "Low-speed" CDC connection between Windows
    XP/Vista and the USB devices. This tiny filter driver resides between
    "usbser.sys" and the USB port driver, and configures pipes for the
    low-speed devices.

    The "lowcdc.vbs" switches its transfer modes. The bulk transfer is fast,
    but is not allowed by the USB 1.1 standard. The interrupt transfer is slow
    (<=1KB/Sec, or <=9600bps), but meets the standard and reduces system load. 
    
    The low-speed bulk transfer fails when a USB 2.0 High-speed Hub connected
    to a PC's USB 2.0 port, because of the transaction translator.


SPECIFICATION
=============
    This patch works as follows:
      Step 1. Configures the interrupt pipes instead of bulk pipes.
      Step 2. Reforms the interrupt pipes to the bulk after the endpoints
              were generated.
      Step 3. Reports the created pipes as the bulk to the upper layer.
      Step 4. Ignores the interface pipe request.
 
    Platform:   Windows 2000/XP/Vista/7. 
    Function:   Configures the CDC endpoints for the low-speed devices.
    Structure:  Kernel mode driver
    Size:       6,400 bytes

    These drivers are designed for AVR-CDC, and may not work properly on other
    "low-speed CDC" devices. Use this driver at your own risk. 


USAGE
=====
    [Files]
            -- Readme.txt            <= This file
            -- License.txt

            -- lowcdc.vbs            <= Switch bulk/interrupt modes

            /raw 
                 -- avrcdc.inf       <= (Windows 2000/XP)

            /w2k 
                 -- lowcdc.inf       <= Windows 2000 (bulk mode only)
                 -- lowcdc.sys

            /xpvista7
                 -- lowcdc.inf       <= Windows XP/Vista/7 x32
                 -- lowcdc.sys

            /vista64
                 -- Readme64.txt
                 -- lowcdc.inf       <= Windows Vista x64
                 -- lowcdc.sys

            /src                     <= Source code

    [Install]
	0. On windows7 64-bit restart with F8 and choose 'Disable driver signature enforcement'
        1. Connect AVR-CDC device to the system. Windows launches the
           New HardwareWizard.
        2. Specify the folder in which "lowcdc.inf" exists, without
           connecting to "Windows Update" nor searching automatically.
           On Windows 2000, use "avrcdc.inf".
           Ignore the warning that the driver is not certified.
        3. Check the virtual COM port number in the Ports section of
           the Device Manager.
        4. Double click "lowcdc.vbs" to switch transfer modes (XP/Vista/7).

    [Uninstall]
        1. Connect AVR-CDC device.
        2. Within the Device Manager window, right click the "Virtual
           Communications Port" on the list, and delete the driver.
           On Vista, check the box to remove the driver completely,
           and skip the step 3.
        3. Remove the "avrcdc.inf/lowcdc.inf"-corresponded "oemX.inf/pnf"
           pair from the "/windows/inf/" folder. (X is a decimal number)
           Search for "AVR-CDC" string in all the oem*.inf files to find
           them.


DEVELOPMENT
===========
    This driver has been developed on WinDDK 6001.18002/7600.16385
    and was based on the sample (src/general/toaster/filter/devlower).
    Copy the source code into /src and build it under the WinDDK environment.


USING lowbulk.sys FOR FREE
==========================
    The lowcdc.sys is published under an Open Source compliant license.
    See the file "License.txt" for details.


    Osamu Tamura @ Recursion Co., Ltd.
    http://www.recursion.jp/avrcdc/

    15 July 2008
    02 February 2009
    20 March 2009
    30 March 2009	-- fixed a shutdown problem.
    06 June 2009     -- combined bulk/interrupt modes.
    24 August 2009   -- fixed to support Win 2000 (bulk mode).
    21 March 2010   -- fixed lowcdc.vbs for comma-decimal point problem (by Jan Chochola).

