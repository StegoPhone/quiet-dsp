//
// rresamp_crcf_example.c
//
// Demonstration of rresamp object whereby an input signal
// is resampled at a rational rate Q/P.
//

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <getopt.h>

#include "liquid.h"

#define OUTPUT_FILENAME "rresamp_crcf_example.m"

// print usage/help message
void usage()
{
    printf("Usage: %s [OPTION]\n", __FILE__);
    printf("  -h            : print help\n");
    printf("  -P <decim>    : resampling rate (decimation),      default: 3\n");
    printf("  -Q <interp>   : resampling rate (interpolation),   default: 5\n");
    printf("  -m <len>      : filter semi-length (delay),        default: 12\n");
    printf("  -s <atten>    : filter stop-band attenuation [dB], default: 60\n");
    printf("  -n <num>      : number of input sample blocks,     default: 120,000\n");
}

int main(int argc, char*argv[])
{
    // options
    unsigned int    P   = 3;        // input rate (interpolation factor)
    unsigned int    Q   = 5;        // output rate (decimation factor)
    unsigned int    m   = 12;       // resampling filter semi-length (filter delay)
    float           As  = 60.0f;    // resampling filter stop-band attenuation [dB]
    unsigned int    n   = 120e3;    // number of input sample blocks

    int dopt;
    while ((dopt = getopt(argc,argv,"hP:Q:m:s:n:")) != EOF) {
        switch (dopt) {
        case 'h':   usage();            return 0;
        case 'P':   P    = atoi(optarg); break;
        case 'Q':   Q    = atoi(optarg); break;
        case 'm':   m    = atoi(optarg); break;
        case 's':   As   = atof(optarg); break;
        case 'n':   n    = atoi(optarg); break;
        default:
            exit(1);
        }
    }

    // validate input
    if (P == 0 || P > 1000) {
        fprintf(stderr,"error: %s, input rate P must be in [1,1000]\n", argv[0]);
        exit(1);
    } else if (Q == 0 || Q > 1000) {
        fprintf(stderr,"error: %s, output rate Q must be in [1,1000]\n", argv[0]);
        exit(1);
    } else if (As < 0.0f) {
        fprintf(stderr,"error: %s, filter stop-band attenuation must be greater than zero\n", argv[0]);
        exit(1);
    } else if (n == 0) {
        fprintf(stderr,"error: %s, number of input samples must be greater than zero\n", argv[0]);
        exit(1);
    }

    // create resampler object
    rresamp_crcf q = rresamp_crcf_create(P,Q,m,As);
    rresamp_crcf_print(q);
    float rate = rresamp_crcf_get_rate(q);
    P          = rresamp_crcf_get_decim(q);
    Q          = rresamp_crcf_get_interp(q);

    // arrays
    float complex buf_x[P];
    float complex buf_y[Q];

    // create signal generator (wide-band noise)
    msourcecf gen = msourcecf_create();
    msourcecf_add_noise(gen, 0.7f * (rate > 1.0 ? 1.0 : rate));

    // create spectral periodogram objects
    unsigned int nfft = 2400;
    spgramcf px = spgramcf_create_default(nfft);
    spgramcf py = spgramcf_create_default(nfft);

    // generate input signal (filtered noise)
    unsigned int i;
    for (i=0; i<n; i++) {
        // write samples to buffer
        msourcecf_write_samples(gen, buf_x, P);

        // run resampler in blocks
        rresamp_crcf_execute(q, buf_x, buf_y);

        // write input and output to respective spectral periodogram estimate
        spgramcf_write(px, buf_x, P);
        spgramcf_write(py, buf_y, Q);
    }
    printf("num samples in  : %llu\n", spgramcf_get_num_samples_total(px));
    printf("num samples out : %llu\n", spgramcf_get_num_samples_total(py));

    // clean up allocated objects
    rresamp_crcf_destroy(q);

    // compute power spectral density output
    float X[nfft];
    float Y[nfft];
    spgramcf_get_psd(px, X);
    spgramcf_get_psd(py, Y);

    // export results to file for plotting
    FILE * fid = fopen(OUTPUT_FILENAME,"w");
    fprintf(fid,"%% %s: auto-generated file\n",OUTPUT_FILENAME);
    fprintf(fid,"clear all;\n");
    fprintf(fid,"close all;\n");
    fprintf(fid,"P    = %u;\n", P);
    fprintf(fid,"Q    = %u;\n", Q);
    fprintf(fid,"r    = P/Q;\n");
    fprintf(fid,"nfft = %u;\n", nfft);
    fprintf(fid,"X    = zeros(1,nfft);\n");
    fprintf(fid,"Y    = zeros(1,nfft);\n");
    for (i=0; i<nfft; i++) {
        fprintf(fid,"X(%3u) = %12.4e;\n", i+1, X[i]);
        fprintf(fid,"Y(%3u) = %12.4e;\n", i+1, Y[i]);
    }
    fprintf(fid,"\n\n");
    fprintf(fid,"%% plot time-domain result\n");
    fprintf(fid,"fx=[0:(nfft-1)]/nfft-0.5;\n");
    fprintf(fid,"fy=fx/r;\n");
    fprintf(fid,"figure('Color','white','position',[500 500 800 600]);\n");
    fprintf(fid,"plot(fx,X,'-','LineWidth',2,'Color',[0.5 0.5 0.5],'MarkerSize',1,...\n");
    fprintf(fid,"     fy,Y,'-','LineWidth',2,'Color',[0.5 0 0],    'MarkerSize',1);\n");
    fprintf(fid,"legend('original','resampled','location','northeast');");
    fprintf(fid,"xlabel('Normalized Frequency [f/F_s]');\n");
    fprintf(fid,"ylabel('Power Spectral Density [dB]');\n");
    fprintf(fid,"fmin = min(fx(   1),fy(   1));\n");
    fprintf(fid,"fmax = max(fx(nfft),fy(nfft));\n");
    fprintf(fid,"axis([fmin fmax -100 20]);\n");
    fprintf(fid,"grid on;\n");
    fclose(fid);
    printf("results written to %s\n",OUTPUT_FILENAME);
    return 0;
}
