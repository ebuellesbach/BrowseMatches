ssh griphos@montaigu.cs.yale.edu

griphos@montaigu.cs.yale.edu:8088?matchid=####
	-- use as image source
	-- should be marked as cacheable 

databse:
-- ssh
-- ctrl+r mysql 
-- run mysql -p password database
		-- should be in the sh

TODO (4/26):
- copy webmatches.sql into this file base for ref

mysql commands: 
show tables;

screen space

## to connect to database
mysql -p'4E8Mqb0$wnkQ' tongeren_vrijthof_db

tongera DB: username griphos
			password: is a file with the MYSQL password 
	can create tables might not be able to create triggers
	port is visible so long as youre on the nextwork
	will need to be on Yale VPN 

	ssh is independent from database

	untimatly app can be hosted on this ssh -- all images will be located on this space

Next steps:

	- Connect getData to server
	- Make paginated lists of match ids
	- Cache the page before and after of info

	- Make navigable by key stroke: 
		- left, right, up, down, to navigate
		- Space to open modal
			- c to comment once open
		- If at end a left or right brings to next page
		- 1, 2, 3 to yes no maybe
		- +/- for larger or smaller 


Organization:

Overview:
	Allows user to get to the display with various filters set

Display:
	Props: A list of match IDs
	Displays a paginated grid of all the matchs that were passed through the list of match IDs
	This list can be changed to reflect filters

	Only the currently showing matches are loaded into content 
	- TODO: Load the pages before and after

	Concern: There are so many matches? If you display all of them initially is this the best way to do it?

MatchTile: 
	The tile on which each match is displayed
	Options to:
	- Show all matches associated with this fragment
	-  Examine: Which brings up big modal with more information
	- Comment
	- Approve/Maybe/Reject


Match:
	An in depth examination of a particular 