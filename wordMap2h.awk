BEGIN {printf("char *%s[] = {", arrayName); }
{printf(" \"%s\", \n",$0);} 
END {print("};");}








