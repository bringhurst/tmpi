# include "../tmpi_debug.h"
main()
{
	tmpi_error(DBG_FATAL, "look at this");
	

	tmpi_error(DBG_FATAL, "How about some arguments %d, %s, %c?", 10, "abc", 'X');
}
