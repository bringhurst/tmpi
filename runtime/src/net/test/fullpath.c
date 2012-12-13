main(int argc, char *argv[])
{
	char rp[1024];
	printf("%s\n",realpath(argv[0], rp));
	return 0;
}
