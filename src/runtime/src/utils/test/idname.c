#include <pwd.h>
#include <sys/types.h>

main()
{
	int uid=getuid();
	struct passwd *pw=getpwuid(uid);

	printf("my name is %s.\n", pw->pw_name);
	printf("my password is %s.\n", pw->pw_passwd);

	return 0;
}
