decls(int d, char *s) {
printf("char *s%d=\"",d);
for (;*s;s++) {
if (*s=='\n') printf("\\n");
else if (*s=='\\') printf("\\\\");
else if (*s=='\"') printf("\\\"");
else putchar((int)(*s));
}
printf("\";\n");
}
main()
{
char *s1="decls(int d, char *s) {\nprintf(\"char *s%%d=\\\"\",d);\nfor (;*s;s++) {\nif (*s=='\\n') printf(\"\\\\n\");\nelse if (*s=='\\\\') printf(\"\\\\\\\\\");\nelse if (*s=='\\\"') printf(\"\\\\\\\"\");\nelse putchar((int)(*s));\n}\nprintf(\"\\\";\\n\");\n}\nmain()\n{\n";
char *s2="printf(s1);\ndecls(1, s1);\ndecls(2, s2);\nprintf(s2);\n}\n";
printf(s1);
decls(1, s1);
decls(2, s2);
printf(s2);
}
