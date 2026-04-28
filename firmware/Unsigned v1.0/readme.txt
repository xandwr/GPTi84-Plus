Unsigned
By Brian Coventry

What is this app?
    For 90% of the people who download this, this will allow you to install
	an operating system other than 2.55 on your 84+ / 84+SE.

Why do you need this?
    If you bought your calculator after about August 2011, the boot code
	on your calculator does not allow you do install an operating system
	lower than 2.55. And frankly, 2.55 sucks.

How do I know if I need this?
    Press [MODE] [ALPHA] [LN]. If it says BOOT Code 1.03, then you need this
	(If it says 1.04, most likely TI updated their boot code because of 
	me and you are going to have to look elsewhere :D)

How do I use this?
    While this app does allow you downgrade your calculator, that's just one 
	of the many things it does. However, in this section, I'm only going 
	to cover downgrading.

1. Send the program to your calculator (if ti-connect fails on you, check out
	my guide at http://bit.ly/qLR2tz)
2. Run the program with Asm(prgmUNSIGNED) ([2nd][0][Down]X6[Enter][PRGM]find it[Enter][Enter])
3. Press [5]
4. Press [1]
5. Type whatever number you want, (it appears in the about screen), just make 
	it less than 65536
6. Press [Enter]
7. Press [Clear] twice
8. Your calculator will now receive any OS you want, even an 83+ one (but don't
	do that)

Can I undo this?
    Yes. To undo it, just run the app and select "4. Remove cert revision"
	your calculator is now good as new. Every single trace of this mod
	has been completely removed.

Will this cause any legal problems?
    No. I don't really see this being a problem. You can do whatever you want to 
	your calculator. In all honesty, the patch I apply isn't serious at all.
	
Will I be able to return my calculator?
    Yes. Like I said, you aren't even changing anything. The people at walmart
	don't care. In all honesty, you could open up your calculator, torch
	the innards, bring it back, and they would give you a new one.

Where is the source?
    Sorry, if I gave you the source, it would be that much easier for TI to
	patch this. However, if you are dying to see it, contact me with some
	sort of evidence that you are not TI and that you will not send it
	to TI, and I will be happy to send it to you.

Why did TI-Connect hang after sending the OS?
    Sorry about that, the way this patch works, TI connect never gets the signal
	that the OS sent. As long as your calculator is working, you can just
	X-out TI-Connect.


;##########################################
	The rest of the program
;##########################################

For those people who want to do more than just downgrade their operating system
	here are some other things you can do.

This program works on the 83+, 83+SE, 84+, and 84+SE. I have no idea what would
	happen on an Nspire, but I can guarentee it won't work.

Due to hardware constraints, this program will clear ram when you exit on an
	83+. I did this on purpose, if you manage to quit without ram clearing,
	you'll notice that your ram is corrupt, and you'll have to clear ram 
	anyways.

Everything in the app is undoable. You can remove every trace just by selecting
	"4. Remove Cert Revision"

Any time you want to modify your certificate, you have to add a number, sorry 
	about that. There's really no way around it. If you don't add a number
	The about screen still tries to display a number, which will make it 
	glitch and just display garbage.


1. Add name
    This was the original reason I made this program, this option allows you
	to put your name, or whatever else you want, in the about screen. There
	is a limit to how many characters you can type, but, I forget what that
	number is. (It's over 40)

2. Add cert revision
    If your only goal in life is to put the cert revision number on your about
	screen, this is the option for you.

3. Remove name
    This will remove your name from the about screen, but will leave everything
	else intact, including any Unsigned patches you added.

4. Remove cert revision
    This removes every thing this app has ever added. And it is the only way
	to get rid of the revision number.

5. Signed / Unsigned
    More info below VVV

6. View cert (Calcsys)
    If you have calcsys installed on your calculator, this will take you
	to the certificate in the hex editor. Since the certificate is only 
	viewable with flash unlocked, flash will be unlocked.


;###############################################
		Signed / Unsigned
;##############################################

Here are my awesome contributions to the calculator world.

1. Unsigned OS's
    This allows you to send any OS you like to your calculator. Anything goes,
	2.43, 2.20, PongOS, OS2, and if you are clever, you might even be able
	to send OS 1.03 to your 84+. (Though it's not going to be happy)

    This also allows you to send modified OS's around. This means that if you
	patch your operating system, you can still send it to other people,
	(as long as they have run this program of course.)

2. Unsigned Apps
    This allows you to send unsigned apps to your calculator. The benefits of
	this come when you have a really really big app, like from TI-Boy, or
	one of the sound/video programs, and it just doesn't want to validate
	on your calculator. This will make it validate.

    Again, you can also send modified apps with this, though, though, there
	aren't many modded apps.

3. Signed OS's
    This un-does #1. If you are done with this program, you might
	as well do number 4 at the main menu.

4. Signed Apps
    This un-does #2. If you are a developer, I recommend keeping your apps signed
	this will allow you to see if your apps will indeed work on other 
	calculators.


;#########################
	Contact
;#########################

If you need to contact me because you love/hate this program:

email me at bcoventry@live.com 
	or
email me at bcoventry77@gmail.com	(this one is newer, but I respond faster)
	or
pm me at omnimaga.org  thepenguin77












































