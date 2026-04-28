Overflow v1.0
Brandon Wilson

>What is it?
This is a couple of programs that I used to unbrick my 84+s and 84+SEs from a very, very, very bad thing.

The short story:
I was playing with modifying the certificate on my calculators (that's another story) and ended up corrupting them by mixing up the endianness of the field sizes of the fields I was messing with. The end result is that when the calculator (meaning _MarkOSInvalid) attempts to parse it and rebuild it (for example, to set a certificate bit), it crashes.
When you have no OS on the calculator and the first thing it must do when receiving a new OS is mark the existing OS invalid in the certificate, that's a big problem.
My calculators were unable to start an OS transfer because the boot code would attempt to call _MarkOSInvalid and crash. Any attempt to circumvent that by messing with the link protocol would not work because all subsequent link packets first check to make sure the OS is marked invalid, and if not, jump to 0000h.

These programs send an extremely large variable header link packet to the boot code, so large that it fills the stack with 90h's and causes the calculator to jump to 9090h. The advantage here is that we can execute code on an OS-less calculator by transferring it over I/O (not direct USB). I used this to erase my certificate so that _MarkOSInvalid wouldn't fail and I could finally resend the TI-OS.

>How do I use it?
Modify code.z80 with the code you want to execute on the bricked calculator (it will run from 9D95h) and re-assemble it to produce CODE.8XP. The included code.z80 contains code to erase the entire certificate and then hang.
Send prgmOVERFLOW and prgmCODE to another calculator and keep them in RAM (you can use the Flash debugger for this).
Attach an I/O cable to both calculators and make sure the bricked calculator is at "Waiting...Please install operating system now."
Run prgmOVERFLOW with Asm(prgmOVERFLOW. It will transfer the link packet in several stages and show the progress (crudely) as it goes. This WILL take a LONG time (5-10 minutes).
When done the sending calculator should simply return with "Done" and the bricked calculator will execute your code.

>What in the world is this for?
Read the description above one more time.

>Why did you release this?
To provide a method for anyone to execute code remotely on a bricked, OS-less calculator. I bricked four calculators by modifying the certificate and this was the only way I could fix them. Hopefully it helps someone.

>What's UOFLOW and how is it different from OVERFLOW?
This is the same thing except it sends a malformed request-to-send packet to send a certificate revision. Some versions of the boot code (mine, at least, and I think probably all of them) won't lock Flash back after receiving this link packet, so if you need to execute code with Flash unlocked, use prgmUOFLOW instead of prgmOVERFLOW (I had to do this since I was erasing the certificate, which requires Flash unlocked first). Your mileage may vary.

>How can I contact you?
brandonw.net, brandonlw@gmail.com, etc.
