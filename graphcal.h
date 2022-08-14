// HG_ZERO (linear power scale) moves the high resolution graph.
// Set for same reading as main graph.
#define HG_ZERO 0.4  
// FFT2_WATERFALL_ZERO  (linear power scale) is the level where the 
// colour scale starts. Set for dark blue when main graph and high 
// resolution graph show the dB value selected as waterfall zero.
// This parameter is in use only when second fft is enabled
#define FFT2_WATERFALL_ZERO 0.012
// move the waterfall zero point
#define WATERFALL_SCALE_ZERO 1

#define FFT1_WATERFALL_ZERO 0.14
// HG_GAIN can be used to change the relation between waterfall gain
// and the y-scale of the high resolution graph.
#define HG_GAIN 0.5


#define FFT1_BASEBAND_FACTOR 1.5

#define FFT2_BASEBAND_FACTOR 0.4
