/*
 * hbkb.h
 *
 *  Created on: Sep 12, 2015, Current Version: 1
 *      Author: jbr373
 *
 *      #############
 *      # GFX CODE BY SMEALUM
 *      # http://www.github.com/smealum/
 *      #############
 *
 *      HBKB (HomeBrewKeyBoard) General Information:
 *
 *      HBKB is intended to be a useful Keyboard for Homebrew
 *      Applications.
 *
 *      Usage:
 *
 *		First of all, you need to include the HBKB Header in
 *		your Project:
 *		#include "hbkb.h"
 *
 *      Checking User Input and drawing the Keyboard:
 *
 *      Use HBKB_CallKeyboard(touchPosition); to call the Keyboard.
 *      The Keyboard Function is run with the current Touch
 *      Position.
 *      The Function will then Check the Current Touch Position and
 *      write the Graphics for the Keyboard on the Bottom Screen
 *      of the Nintendo 3DS.
 *
 *      NOTE THAT THE WHOLE BOTTOM SCREEN WILL BE OCCUPIED AND THAT
 *      THIS LIBRARY DOES NOT CALL THE GFX FLUSH AND SWAP FUNKTIONS!
 *
 *      The HBKB_CallKeyboard Function will add Letters based on the
 *      Touch Position to a String.
 *
 *      HBKB_CallKeyboard will after it was called return a Value (u8) to
 *      inform about the current User actions.
 *      1 = User pressed Enter
 *      2 = User pressed any other Key except OK / Cancel
 *      3 = User pressed Cancel
 *      4 = User pressed no Key.
 *
 *      0 = Unknown (Something went wrong)
 *
 *      Also, the way this works is that it checks for Input first,
 *      then draws the Keyboard. This is due to the drawing of the
 *      Graphic for the selected Key(s).
 *
 *      Requesting User Input:
 *
 *      HBKB_CheckKeyboardInput() returns a std::string with the Input
 *      the User made during the time the Keyboard was called.
 *      This string is => NOT <= reset after HBKB_CallKeyboard() returned it's
 *      u8 Value.
 *
 *      NOTE THAT THIS STRING IS NOT DRAWN BY HBKB! YOU NEED TO CHECK THIS
 *      STRING AND DRAW IT YOURSELF!
 *
 *      Reset User Input:
 *
 *      You can reset the Keyboard with HBKB_Clean().
 *
 *
 */

#ifndef HBKB_H_
#define HBKB_H_

#include <3ds.h>
#include <string>

class HB_Keyboard
{
public:
	HB_Keyboard();
	virtual ~HB_Keyboard();

	// Call Keyboard
	u8 HBKB_CallKeyboard(touchPosition TouchScreenPos);

	// Return User Input
	std::string HBKB_CheckKeyboardInput();

	// Clean User Input
	void HBKB_Clean();

private:
	void KeyInteraction(u8 &Key);
	void ChangeString(u8 &Key);
	void GFXBufferInteraction();

	std::string UserInput;
	bool isShift;
	bool isCaps;

	u8 KeyboardState;
	u8 CurrentKey;
};

#endif /* HBKB_H_ */
