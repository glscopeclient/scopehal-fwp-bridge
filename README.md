# FastWavePort Bridge for Teledyne LeCroy Oscilloscopes

This utility is a bridge server for high performance, push based access to raw waveform data on Teledyne LeCroy oscilloscopes which have the XDEV software option installed.

It provides a TCP server on port 1862 which supports one client at a time.

If you are using the "lecroy_fwp" driver in libscopehal to talk to this bridge, all instrument-side configuration is automatic. Simply start the bridge server on the oscilloscope and the driver will configure the FastWavePort blocks as needed.

## Client to server protocol

Clients may subscribe to up to four analog channels worth of data. Digital channels and 8-channel scopes are not currently supported.

To update your set of subscriptions, send a single byte to the server. The high nibble is ignored; the low nibble is treated as a bitmask where 1 = channel 1, 2 = channel 2, 4 = channel 3, 8 = channel 4. A high in each bit position indicates the client is interested in data from this channel and a 0 indicates it is not interested.

## Server to client protocol

Each time the scope triggers, the server sends a two part data block to the client consisting of a fixed sized header and variable sized per-channel waveform data.

The header consists of four consecutive CDescHeader structures (as documented in the "Fast Wave Port Function" section of the Teledyne LeCroy Advanced Customization Instruction Manual) for channels 1-4 in sequence. If a channel is subscribed, the CDescHeader block will contain the raw waveform headers as provided via FastWavePort. If the channel is inactive, an all-zeroes CDescHeader is sent instead as a placeholder.

After the headers, each channel's data is sent. The data block consists of header.numSamples signed 16-bit integers, and is copied verbatim from the FastWavePort shared memory without any preprocessing.

## Instrument-side configuration

This server must be run on the oscilloscope itself, since it uses Windows shared memory to communicate with MAUI. A FastWavePort math function must be configured for each active channel; the port name for the math function must be "FastWavePortN" where N is the channel number.

Any math channel may be used for the FastWavePort blocks; the "lecroy_fwp" driver in libscopehal uses F9-F12. Note that the FastWavePort function is synchronous and will block until a timeout elapses; it is thus important to ensure that the set of enabled FastWavePort math channels matches the set of channels the client is subscribed to (since scopehal-fwp-bridge only acknowledges data on active channels). The "lecroy_fwp" driver manages this automatically but if you are writing custom client software it will be necessary to send SCPI or Automation commands to the instrument to do this.
