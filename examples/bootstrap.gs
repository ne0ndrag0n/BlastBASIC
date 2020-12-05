#
# bootstrap.gs
# Sega Megadrive bootstrap file
# Author: Ashley N.
# Date: 2020 Nov 20
#

import gs/genesis/constants.gs
import gs/genesis/vdp.gs
import gs/genesis/echo.gs

type Statistics
	def ticks as u32 = 0
end

# Variables placed outside of functions are defined in the "data" segment as globals.
def statistics as Statistics

# This is a *compiler directive* which tells GoldScorpion how to use a given block of code.
# When declaring the "header" module, you must follow it with a set number of constants of 
# certain types, in order. Subsequent header declarations throw a compiler error, as does
# the absence of the "header" module.
@[header]
const CONSOLE_MODE as string = 		"EVERDRIVE"
const COPYRIGHT_TAG as string =     "ASHN"
const COPYRIGHT_YEAR as string = 	"2020"
const COPYRIGHT_MONTH as string =   "APR"
const DOMESTIC_NAME as string =     "Concordia - The World of Harmony"
const INTL_NAME as string =         "Concordia - The World of Harmony"
const SERIAL_NO as string =         "GM 20180701-00"
const IO_SUPPORT as string =        "J"
const MODEM_INFO as string =        ""
const COMMENT as string =           ""
const LOCALIZATION as string =      "JUE"

# The "irq" module lets you set a function to be called when the target encounters an IRQ.
@[irq=6]
function vblank()
	statistics.ticks = statistics.ticks + 1
end

# This code is inserted directly after bootstrap code inserted by the compiler.
# Put routines you want to run at system startup here.
@[code_start]
function reset()
	def i as u16

	Vdp.SetCramWrite()
	for i = 0 to $3f
		Vdp.Write( 0 )
	end

	Vdp.SetVramWrite()
	for i = 0 to $7fff
		Vdp.Write( 0 )
	end

	for i = 0 to Constants.DATA_SEG_MAX
		# Variables placed inside functions are defined on the stack when they are declared.
		def blank as u8 = 0
	end

	Echo.Init()
end