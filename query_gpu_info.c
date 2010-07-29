/*
 * nvidia-xconfig: A tool for manipulating X config files,
 * specifically for use by the NVIDIA Linux graphics driver.
 *
 * Copyright (C) 2006 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *      Free Software Foundation, Inc.
 *      59 Temple Place - Suite 330
 *      Boston, MA 02111-1307, USA
 *
 *
 * query_gpu_info.c
 */

#include "nvidia-xconfig.h"
#include <string.h>

static char *display_device_mask_to_display_device_name(unsigned int mask);


#define TAB    "  "
#define BIGTAB "     "
#define BUS_ID_STRING_LENGTH 32


/*
 * query_gpu_info() - query information about the GPU, and print it
 * out
 */

int query_gpu_info(Options *op)
{
    DevicesPtr pDevices;
    DisplayDevicePtr pDisplayDevice;
    int i, j;
    char *name, busid[BUS_ID_STRING_LENGTH];

    /* query the GPU information */

    pDevices = find_devices(op);
    
    if (!pDevices) {
        fmterr("Unable to query GPU information");
        return FALSE;
    }
    
    /* print the GPU information */

    fmtout("Number of GPUs: %d", pDevices->nDevices);

    for (i = 0; i < pDevices->nDevices; i++) {
        
        fmtout("");
        fmtout("GPU #%d:", i);
        fmtoutp(TAB, "Name      : %s", pDevices->devices[i].name);
        
        memset(busid, 0, BUS_ID_STRING_LENGTH);
        xconfigFormatPciBusString(busid, BUS_ID_STRING_LENGTH,
                                  pDevices->devices[i].dev.domain,
                                  pDevices->devices[i].dev.bus,
                                  pDevices->devices[i].dev.slot, 0);
        fmtoutp(TAB, "PCI BusID : %s", busid);

        fmtout("");
        fmtoutp(TAB, "Number of Display Devices: %d",
                pDevices->devices[i].nDisplayDevices);
        fmtout("");

        for (j = 0; j < pDevices->devices[i].nDisplayDevices; j++) {

            pDisplayDevice = &pDevices->devices[i].displayDevices[j];

            name = display_device_mask_to_display_device_name
                (pDisplayDevice->mask);
            
            if (!name) name = nvstrdup("Unknown");

            fmtoutp(TAB, "Display Device %d (%s):", j, name);
        
            nvfree(name);

            /*
             * convenience macro to first check that the value is
             * non-zero
             */
            
            #define PRT(_fmt, _val)                  \
                if (_val) {                          \
                    fmtoutp(BIGTAB, (_fmt), (_val)); \
                }
            
            if (pDisplayDevice->info_valid) {
                
                PRT("EDID Name             : %s", 
                    pDisplayDevice->info.monitor_name);
                
                PRT("Minimum HorizSync     : %.3f kHz",
                    pDisplayDevice->info.min_horiz_sync/1000.0);
                
                PRT("Maximum HorizSync     : %.3f kHz",
                    pDisplayDevice->info.max_horiz_sync/1000.0);
                
                PRT("Minimum VertRefresh   : %d Hz",
                    pDisplayDevice->info.min_vert_refresh);
                
                PRT("Maximum VertRefresh   : %d Hz",
                    pDisplayDevice->info.max_vert_refresh);
                
                PRT("Maximum PixelClock    : %.3f MHz",
                    pDisplayDevice->info.max_pixel_clock/1000.0);
                
                PRT("Maximum Width         : %d pixels",
                    pDisplayDevice->info.max_xres);
                
                PRT("Maximum Height        : %d pixels",
                    pDisplayDevice->info.max_yres);
                
                PRT("Preferred Width       : %d pixels",
                    pDisplayDevice->info.preferred_xres);
                
                PRT("Preferred Height      : %d pixels",
                    pDisplayDevice->info.preferred_yres);
                
                PRT("Preferred VertRefresh : %d Hz",
                    pDisplayDevice->info.preferred_refresh);
                
                PRT("Physical Width        : %d mm",
                    pDisplayDevice->info.physical_width);
                
                PRT("Physical Height       : %d mm",
                    pDisplayDevice->info.physical_height);
                
            } else {
                fmtoutp(BIGTAB, "No EDID information available.");
            }
            
            fmtout("");
        }
    }
    
    free_devices(pDevices);

    return TRUE;
    
} /* query_gpu_info() */




/*
 * diaplay_mask/display_name conversions: the NV-CONTROL X extension
 * identifies a display device by a bit in a display device mask.  The
 * below function translates from a display mask to a string
 * describing the display devices.
 */

#define BITSHIFT_CRT  0
#define BITSHIFT_TV   8
#define BITSHIFT_DFP 16

#define BITMASK_ALL_CRT (0xff << BITSHIFT_CRT)
#define BITMASK_ALL_TV  (0xff << BITSHIFT_TV)
#define BITMASK_ALL_DFP (0xff << BITSHIFT_DFP)

/*
 * display_device_mask_to_display_name() - construct a string
 * describing the given display device mask.
 */

#define DISPLAY_DEVICE_STRING_LEN 256

static char *display_device_mask_to_display_device_name(unsigned int mask)
{
    char *s;
    int first = TRUE;
    unsigned int devcnt, devmask;
    char *display_device_name_string;

    display_device_name_string = nvalloc(DISPLAY_DEVICE_STRING_LEN);

    s = display_device_name_string;

    devmask = 1 << BITSHIFT_CRT;
    devcnt = 0;
    while (devmask & BITMASK_ALL_CRT) {
        if (devmask & mask) {
            if (first) first = FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "CRT-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }

    devmask = 1 << BITSHIFT_DFP;
    devcnt = 0;
    while (devmask & BITMASK_ALL_DFP) {
        if (devmask & mask)  {
            if (first) first = FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "DFP-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    devmask = 1 << BITSHIFT_TV;
    devcnt = 0;
    while (devmask & BITMASK_ALL_TV) {
        if (devmask & mask)  {
            if (first) first = FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "TV-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    *s = '\0';
    
    return (display_device_name_string);

} /* display_device_mask_to_display_name() */
