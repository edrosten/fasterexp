BEGIN{
	print "const double exps_0_16_256[257]="
	print "{"

	for(i=0; i <= 256; i++)
		print "exp("  0- i * 16 / 256  "),"

	print "};"


}
