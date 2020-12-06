import gs/genesis/debug.gs

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
function x() as u8
	def i as u8

	return i
end
