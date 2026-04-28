# 83Plus:BCALLs:4A14

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4A14

## Synopsis

**Official Name:** SendVarCmd

**BCALL Address:** 4A14

"Sends a variable to a computer via a silent link request."

## Inputs

- OP1: type and name of the variable to transmit

## Outputs

None.

## Destroys

All registers.

## Comments

- Operates silently and does not generate error messages on failure.
- **Will fail to transmit if the receiving device has machine ID 82h, 83h, or 73h.**
- Despite using machine ID 82h when initiating contact, "refuses to talk to any calculator."
- Uses a silent link request mechanism, similar in behavior to SendScreenshot.

**Note for our use case:** this is the bcall the calc currently uses to
push CHATMSG to the Pico. The Pico must present a machine ID OTHER than
82h/83h/73h to receive `_SendVarCmd` traffic. (We're using 0x73 for the
DBUS-level handshake; reconfirm what ID actually appears in the
`_SendVarCmd` packet header.)
