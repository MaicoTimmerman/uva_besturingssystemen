Een korte beschrijving van de beschikbare bestanden:
README -	dit readme bestand
isam.c -	de sources voor de isam bibliotheek routines
isam.h -	de bijbehorende header file
index.c -	de sources voor de routines die de index voor de isam file
		verzorgen.
index.h -	de bijbehorende header file.
isam_bench.c -  een testprogramma voor de isam routines, ook bedoeld als
		benchmark.
namen, initialen, titles - drie invoerfiles voor gebruik met isam_bench
		Gebruik:
		isam_bench namen initialen titels
		isam_bench namen initialen titels debug
isam_test.c -   een ander testprogramma
refs.txt - invoer voor isam_test, te gebruiken als
		isam_test refs.isam < refs.txt
		hiermee kan een eerste vulling voor tele.isam worden aangemaakt.
Makefile	Makefile. Gebruik b.v.:
		make isam_bench
