BEGIN{
	print "const double exps_0_16_256[257]="
	print "{"
	for(i=0; i <= 256; i++)
		print "exp("  0- i * 16 / 256  "),"
	print "};"

	print "const double exps_0_16_1024[1025]="
	print "{"
	for(i=0; i <= 1024; i++)
		print "exp("  0- i * 16 / 1024  "),"
	print "};"

	print "const double exps_0_16_4096[4097]="
	print "{"
	for(i=0; i <= 4096; i++)
		print "exp("  0- i * 16 / 4096  "),"
	print "};"
}
