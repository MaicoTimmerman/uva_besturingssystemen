/* A simple program to benchmark an isam file system
   -------------------------------------------------
Author: G.D. van Albada
University of Amsterdam
Faculty of Science
Informatics Institute
dick at science.uva.nl
Version: 0.0
Date: January 2002 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mt19937.h"


#include "isam.h"


typedef struct KLANT {
    char    naam[64];
    char    voorvoegsel[16];
    char    initialen[16];
    char    titel[16];
    unsigned long klantSinds; /* Dag na 1/1/1900 */
    unsigned long laatsteMailing; /* Dag na 1/1/1900 */
    unsigned long laatsteBestelling;
    unsigned long nummerBestelling;
}
klant;

static
char    initialen[500][16];

static
int     Ninit = 0;

typedef struct NAAM {
    char    naam[64];
    char    voorvoegsel[16];
} naam;

static
naam    namen[500];

static
int     Nnamen = 0;

static
char    titels[64][16];

static
int     Ntitels = 0;

static
char    sleutels[9000][20];

static
int     Nsleutels = 0;

static
int     report = 0;

/* Bereken dagnummer met 1/1/1900 == 1 */
/* Routine faalt op en na 1/3/2100     */

static long berekenDag (int dag, int maand, int jaar) {
    long    dagen;

    dagen = dag;
    dagen += 365 * (jaar - 1900) + (jaar - 1901) / 4;
    switch (maand) {
        case 12:
            dagen += 30;
        case 11:
            dagen += 31;
        case 10:
            dagen += 30;
        case 9:
            dagen += 31;
        case 8:
            dagen += 31;
        case 7:
            dagen += 30;
        case 6:
            dagen += 31;
        case 5:
            dagen += 30;
        case 4:
            dagen += 31;
        case 3:
            dagen += ((0 == jaar % 4) && jaar > 1900) ? 29 : 28;
        case 2:
            dagen += 31;
        case 1:
            break;
        default:
            return -1;
    }
    return dagen;
}

/* Print klant-informatie */

static void printKlant (FILE * log, char *kop, char *sleutel, klant * Klant) {
    if (kop) {
        fprintf (log, "%s\n", kop);
    }
    fprintf (log, "%s \t||%s %s %s %s\t%lu\t%lu\t%lu\t%lu\n",
            sleutel, Klant->titel, Klant->initialen,
            Klant->voorvoegsel, Klant->naam,
            Klant->klantSinds, Klant->laatsteMailing,
            Klant->laatsteBestelling, Klant->nummerBestelling);
}

/* Maak een random klant */

static void maakKlant (klant * nieuweKlant) {
    int     i;
    int     dag, maand, jaar;

    if (Nnamen) {
        i = genrand_int31 () % Nnamen;
        strncpy (nieuweKlant->naam, namen[i].naam, 64);
        strncpy (nieuweKlant->voorvoegsel, namen[i].voorvoegsel, 16);
    }
    if (Ninit) {
        i = genrand_int31 () % Ninit;
        strncpy (nieuweKlant->initialen, initialen[i], 16);
    }
    if (Ntitels) {
        i = genrand_int31 () % Ntitels;
        strncpy (nieuweKlant->titel, titels[i], 16);
    }
    jaar = 1950 + genrand_int31 () % 52;
    dag = genrand_int31 () % 29;
    maand = 1 + genrand_int31 () % 12;
    nieuweKlant->klantSinds = berekenDag (dag, maand, jaar);
    jaar = 2001;
    nieuweKlant->laatsteMailing = berekenDag (dag, maand, jaar);
    jaar = 1998 + genrand_int31 () % 4;
    nieuweKlant->laatsteBestelling = berekenDag (dag, maand, jaar);
    nieuweKlant->nummerBestelling = 100 * nieuweKlant->laatsteBestelling + genrand_int31 () % 95;
    return;
}

static int code = 1000;

/* Maak een random sleutel, dichtbij de postcode in "code". Verhoog code */
/* Sleutel is een postcode plus huisnummer */

static int maakSleutel (char *sleutel) {
    int     huisnummer;

    code += genrand_int31 () % 8;

    /* maak een huisnummer tot 53, met een voorkeur voor lage nummers */

    huisnummer = genrand_int31 () % 30 + genrand_int31 () % 25;
    huisnummer -= genrand_int31 () % 30 + genrand_int31 () % 25;
    huisnummer = (huisnummer <= 0) ? (1 - huisnummer) : huisnummer;

    sprintf (sleutel, "%4d %c%c - %4d", code, (char) ('A' + genrand_int31 () % 26),
            (char) ('A' + genrand_int31 () % 26), huisnummer);
    return (code < 9999);
}

/* Schrijf een nieuw record op een willekeurige plek in het bestand */

static int randomNieuwRecord (isamPtr ip) {
    char    sleutel[20];
    klant   nieuweKlant;
    int     rv;

    memset(&nieuweKlant, 0, sizeof(nieuweKlant));
    memset(sleutel, 0, sizeof(sleutel));
    do {
        code = 1000 + genrand_int31 () % 8990;
        maakSleutel (sleutel);
    } while (0 == isam_readByKey (ip, sleutel, &nieuweKlant));
    maakKlant (&nieuweKlant);
    rv = isam_writeNew (ip, sleutel, &nieuweKlant);
    if (rv && report) {
        isam_perror ("write new client record");
    } else {
        strncpy (sleutels[Nsleutels], sleutel, 20);
    }
    Nsleutels++;
    return Nsleutels;
}

/* Lees sequentieel alle records in bepaald sleutelbereik, en pleeg
   een bewerking */
static int leesBereik (isamPtr ip, char *minSleutel, char *maxSleutel, int datum) {
    char    sleutel[20];
    char    vorigeSleutel[20] = "";
    int     i = 0;
    int     j = 0;
    int     rv;
    klant   klantRecordOud;
    klant   klantRecordNieuw;


    /* Net als bij het plegen van een mailing */
    isam_setKey (ip, minSleutel);
    do {
        rv = isam_readNext (ip, sleutel, &klantRecordOud);
        if (rv) {
            isam_perror ("reading next record");
            break;
        }
        if (strncmp (sleutel, vorigeSleutel, 20) <= 0) {
            printf ("Sleutels staan verkeerd gesorteerd: %s komt voor %s\n",
                    vorigeSleutel, sleutel);
        }
        if (strncmp (sleutel, maxSleutel, 20) > 0)
            break;
        i++;
        if (report) {
            printKlant (stderr, NULL, sleutel, &klantRecordOud);
        }
        if ((int) klantRecordOud.laatsteMailing < datum - 200) {
            klantRecordNieuw = klantRecordOud;
            klantRecordNieuw.laatsteMailing = datum;

            /* Nog te controleren:
               Lees ik na een update wel het direct volgende record?
               */

            rv = isam_update (ip, sleutel, &klantRecordOud,
                    &klantRecordNieuw);
            if (rv) {
                isam_perror ("updating record");
                break;
            }
            j++;
        }
    }
    while (strncmp (sleutel, maxSleutel, 20) <= 0);
    printf ("Er zijn %d records gelezen; mailings zijn verzonden aan %d\n", i, j);
    return i;
}

int leesBestaandRecord (isamPtr ip, int sleutelNr) {
    klant   klantRecord;
    int     rv;

    /* Als klantgegevens uitvragen */

    rv = isam_readByKey (ip, sleutels[sleutelNr], &klantRecord);
    if (rv && report)
    {
        isam_perror ("reading a record by key");
    }
    return rv;
}



int poetsBestaandRecord (isamPtr ip, int sleutelNr) {
    klant   klantRecord;
    int     rv;

    /* Als klantgegevens wissen */

    rv = isam_readByKey (ip, sleutels[sleutelNr], &klantRecord);
    if (rv) {
        if (report)
            isam_perror ("reading a record by key");
        return rv;
    }
    rv = isam_delete (ip, sleutels[sleutelNr], &klantRecord);
    if (rv) {
        isam_perror ("deleting a record by key");
    }
    return rv;
}

int main (int argc, char *argv[]) {
    isamPtr ip;
    FILE   *inp;
    char    sleutel[20] =
    {0};
    klant   nieuweKlant;
    char    str[512];
    int     i, j;

    memset(&nieuweKlant, 0, sizeof(nieuweKlant));
    /* Lees het bestand met namen (en voorvoegsels) */

    init_genrand(171717);
    if (argc < 4) {
        printf ("Gebruik: %s namen initialen titels [optional-debug]\n", argv[0]);
        return -1;
    }
    if (argc == 5) {
        report = 1;
        printf ("Produceer meer debug uitvoer\n");
    }
    inp = fopen (argv[1], "r");
    if (inp) {
        i = 0;
        printf ("Lees namen van %s\n", argv[1]);
        while (fgets (str, 512, inp)) {
            strncpy (namen[i].naam, strtok (str, ";\t\n"), 64);
            strncpy (namen[i].voorvoegsel, strtok (NULL, ";\t\n"), 16);
            i++;
        }
        fclose (inp);
        Nnamen = i;
    } else {
        perror ("Failed to open names file");
        exit (-1);
    }

    /* Lees het bestand met initialen */

    inp = fopen (argv[2], "r");
    if (inp) {
        i = 0;
        printf ("Lees initialen van %s\n", argv[2]);
        while (fgets (str, 512, inp)) {
            strncpy (initialen[i], strtok (str, "\n"), 16);
            i++;
        }
        fclose (inp);
        Ninit = i;
    } else {
        perror ("Failed to open initials file");
        exit (-1);
    }

    /* Lees het bestand met titels */

    inp = fopen (argv[3], "r");
    if (inp) {
        i = 0;
        printf ("Lees titels van %s\n", argv[3]);
        while (fgets (str, 512, inp)) {
            strncpy (titels[i], strtok (str, "\n"), 16);
            i++;
        }
        fclose (inp);
        Ntitels = i;
    } else {
        perror ("Failed to open titles file");
        exit (-1);
    }

    /* Probeer een isam bestand aan te maken. Als dat mislukt,
       bestaat het mogelijk al - probeer het dan te lezen */

    ip = isam_create ("klant.isam", 20, sizeof (klant), 8, 360);
    if (!ip) {
        /* Mislukt ... bestaat het al ? */

        isam_perror ("Failed to create file");
        ip = isam_open ("klant.isam", 1);
        if (!ip) {
            isam_perror ("Failed to open file");
            exit (-1);
        }

        /* Lees alle sleutels */

        i = 0;
        while (0 == isam_readNext (ip, sleutels[i], &nieuweKlant)) {
            i++;
        }
        Nsleutels = i;
        printf ("Bestaand bestand met %d records geopend\n", Nsleutels);
    } else {
        /* Vul het bestand, min of meer sequentieel */

        i = 0;
        while (maakSleutel (sleutel)) {
            maakKlant (&nieuweKlant);
            if (isam_writeNew (ip, sleutel, &nieuweKlant)) {
                isam_perror ("Failed to create customer record");
            } else {
                if (report) {
                    printKlant (stdout, NULL, sleutel, &nieuweKlant);
                }
                strncpy (sleutels[i], sleutel, 20);
                i++;
            }
        }
        printf ("Isam bestand bevat %d records\n", i);

        Nsleutels = i;

        /* Dit hoeft niet, maar test open en close */

        isam_close (ip);
        ip = isam_open ("klant.isam", 1);
    }

    /* Doe een aantal bewerkingen op het bestand -
       lees sequentieel plus update
       lees op sleutel
       poets
       schrijf nieuw
       */
    leesBereik (ip, "2300", "4500", berekenDag (25, 1, 2002));
    for (i = 0, j = 0; i < 500; i++) {
        if (leesBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
    }
    printf ("%d records gelezen, %d mislukt\n", i, j);
    for (i = 0, j = 0; i < 200; i++) {
        if (poetsBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
        randomNieuwRecord (ip);
    }
    printf ("%d nieuwe klanten, %d vertrokken\n", i, i - j);
    leesBereik (ip, "2300", "4500", berekenDag (25, 1, 2002));
    for (i = 0, j = 0; i < 500; i++) {
        if (leesBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
    }
    printf ("%d records gelezen, %d mislukt\n", i, j);
    for (i = 0, j = 0; i < 200; i++) {
        if (poetsBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
        randomNieuwRecord (ip);
    }
    printf ("%d nieuwe klanten, %d vertrokken\n", i, i - j);
    leesBereik (ip, "2300", "4500", berekenDag (25, 1, 2002));
    for (i = 0, j = 0; i < 500; i++) {
        if (leesBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
    }
    printf ("%d records gelezen, %d mislukt\n", i, j);
    for (i = 0, j = 0; i < 200; i++) {
        if (poetsBestaandRecord (ip, genrand_int31 () % Nsleutels)) {
            j++;
        }
        randomNieuwRecord (ip);
    }
    printf ("%d nieuwe klanten, %d vertrokken\n", i, i - j);
    leesBereik (ip, "1000", "9999", berekenDag (25, 1, 2002));
    leesBereik (ip, "1000", "9999", berekenDag (25, 1, 2002));
    isam_close (ip);

    return 0;
}
