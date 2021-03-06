#include "modem_iqwav.h"
#include <math.h>

ModemIQWav::ModemIQWav(long frequency, long bandwidth, std::string outputFile)
{
    // Wav output & local variables
    frequency_m = frequency;
    bandwidth_m = bandwidth;
    tinywav_open_write(&outWavFile, 2, bandwidth, TW_INT16, TW_INTERLEAVED, outputFile.c_str());
}

void ModemIQWav::stop()
{
    // Close wav
    tinywav_close_write(&outWavFile);
}

void ModemIQWav::process(liquid_float_complex *buffer, unsigned int &length)
{
    // Write samples to output file
    unsigned int bufferOutSize = length * 2;
    float outputBuffer[bufferOutSize];

    for (size_t i = 0; i < length; i++)
    {
        outputBuffer[i * 2] = buffer[i].real();
        outputBuffer[i * 2 + 1] = buffer[i].imag();
    }

    tinywav_write_f(&outWavFile, outputBuffer, bufferOutSize);
}