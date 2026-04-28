# 83Plus:OS:84_Plus_USB_Information

Source: https://wikiti.brandonw.net/index.php?title=83Plus:OS:84_Plus_USB_Information

## Summary

WikiTI overview of USB on the 84+ / 84+ SE: software entry points, hardware
ports, RAM locations, and USB protocol details.

## Key sections (per the live page)

- **Easy Data Functions:** five entry points (5254, 5257, 525A, 525D, 5260) used during USB init/shutdown, plus 5290 (USB init) and 5293 (LED control).
- **Special App Header:** devices can trigger automatic app launches via a header at 4087h, containing a table of USB device identifiers the OS monitors.
- **Hardware Ports:** 30+ I/O ports (4C through A0+) used for USB : status registers, data pipes, frame counters, control registers.
- **Memory Locations:** RAM 9C13-9C79 stores vendor/product IDs, device state, buffers, configuration data.
- **Peripheral Mode:** the calculator can act as a USB peripheral and respond to standard device class requests (Set Address, Set Configuration, Get Descriptor).

## Notable findings

- "USB auto-launch" occurs when compatible devices connect *if* the home screen is active and no program is running.
- Successful testing reported with optical mice and digital cameras.
- References the external `usb8x` driver project for broader device compatibility.

## Notes for our use case

Not directly relevant to the calc-master REQ work over DBUS (the link-port
domain). Saved here because the user pointed at it, and because it's the
right reference if we ever revisit the USB path :  the project memory locks
us to DBUS, but USB is the documented alternative if that lock ever lifts.
For reproducibility, fetch the live page when working on USB code; the
WebFetch summary is intentionally lossy.
