import gs/genesis/debug.gs

const SOME_CONST as string = "This is a constant string"

def i as u8 = 42

if i != 42 then
	Debug.print( "Fix your code" )
else if i == 666 then
	Debug.print( "zomg demon possessed" )
else if i == 777 then
	Debug.print( "z?" )
else
	Debug.print( "Boop" )
	Debug.print( "Everything works" )
end

@[expr=value]
function x() as u16
	def i as u16 = 6

	asm
		move.w #0, (sp)
	end

	while i != 6
		i = 0
	end

	# i should return as 0 after the inline asm
	return i
end
