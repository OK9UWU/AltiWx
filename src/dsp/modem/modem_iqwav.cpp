#include "modem_iqwav.h"
#include <math.h>

ModemIQWav::ModemIQWav(long frequency, long bandwidth, std::string outputFile)
{
    tinywav_open_write(&outWavFile, 2, bandwidth, TW_INT16, TW_INTERLEAVED, outputFile.c_str());
}

void ModemIQWav::stop()
{
    tinywav_close_write(&outWavFile);
}

void ModemIQWav::process(liquid_float_complex *buffer, unsigned int &length)
{
    unsigned int bufferOutSize = length * 2;
    float outputBuffer[bufferOutSize];

    for (size_t i = 0; i < length; i++)
    {
        outputBuffer[i * 2] = buffer[i].imag;
        outputBuffer[i * 2 + 1] = buffer[i].real;
    }

    tinywav_write_f(&outWavFile, outputBuffer, bufferOutSize);
}