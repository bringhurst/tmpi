# include <stdio.h>
# include "../machine.h"
main()
{
	printf("cpu current = %d, cpu number = %d\n", cpu_current(), cpu_total());
}
