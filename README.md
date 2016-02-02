## Silent Way for JACK/Linux

The [Expert Sleepers](http://expert-sleepers.co.uk) Eurorack
products allow digital audio outputs from computers to be
injected as analog control voltages and gates into Eurorack
synthesizers.  Several of their products allow many digital
signals to be multiplexed onto a smaller number of S/PDIF or ADAT
digital audio connections.  On MacOS and Windows machines, there's
a vendor-provided driver that manages the multiplexing.  Not so
for Linux.

Luckily the nice folks at Expert Sleepers pointed me to the code
of a Pure Data external implementing the multiplexing:
https://github.com/expertsleepersltd/externals

Based on the algorithms in that code, I'm implmenting a
standalone JACK client to sit between your signal-generating
software and the audio interface that's connected to the Expert
Sleepers device(s).

## Installation

`This code uses "waf" as its build tool.

```
$ ./waf configure build install
```


## Run-time Configuration 

The `silentway` client looks for a configuration file, called
`~/.silentway` by default (a different file can be used via
command-line arguments).  The config file defines what Expert
Sleepers hardware you have and the way it is connected, so that
the application's audio and MIDI ports can be set up correctly. 


## Silent Way overview

Here's what I have figured out about the Silent Way approach
based on my reading of the source code and the Expert Sleepers
documentation.  

### Signal acquisition

Digital audio enters the Expert Sleepers domain via either ADAT
(ES-3) or S/PDIF (ES-4, ES-40).  

Audio channels can be brought out directly to the front panel of
an ES-3; those require no special software so I'm not concerned
with them.  An ES-4/ES-40 S/PDIF interface, or ES-3 channels sent
to an ES-5 expansion interface treat the audio differently.

A single stereo pair of  digital signals (either a stereo 20-bit
S/PDIF connection or 2 channels of a 24-bit ADAT connection) is
treated as a 40- or 48-bit-wide data stream running at the
specified interface rate (generally 44.1 kHz or 48 kHz).  

### Stream splitting

The 40- or 48-bit stream is then split into 5 or 6 8-bit streams
at audio rate. Each of these streams appears on the PCB of the
device (ES-5, ES-4, ES-40) as a 10-pin header marked "GT-1",
"GT-2", etc.  

For the 48-bit stream from an ES-3 into an ES-5 expansion
connector, each 24-bit word is split into 3 8-bit chunks.
Channels 1, 2, and 3 are on the left channel, while 4, 5, and 6
are on the right.

For the 40-bit stream from an ES-4 or ES-40, channels 1 and 2 are
unpacked from the highest 16 bits of the left input channel,
channels 3 and 4 are unpacked from the highest 16 bits of the
right input channel, and channel 5 is assembled from the lowest 4
bits of each (lowest 4 bits of left channel becomes highest 4
bits of channel 5, lowest 4 bits of right becomes lowest 4 bits of
5).

### Expansion devices 

The 8-bit expansion connectors can go to one of several "ESX" 
devices to produce gate or CV outputs: 

 * ESX-8GT just directly outputs the 8 bits as 8 gate/trigger
   signals
 * ESX-4CV uses an encoding scheme to multiplex 4 12-bit CV
   signals at 1/8 the sample rate onto one 8-bit connection
 * ESX-8CV uses a different encoding scheme to multiplex 
   a variable number of 12-bit CV signals (1-8) onto one 
   8-bit connection at a sample rate dependent on the number 
   of connections
 * ESX-8MD generates MIDI/DIN sync signals, I'm not sure how 
   the connection works 

## Encodings 

### Audio to expansion ports 

**ES-3 --> ES-5**: Each 24-bit digital audio signal is split into
3 8-bit segments.  
```
Bitmask (L R)       Channel 
0xff0000 0x000000   1
0x00ff00 0x000000   2
0x0000ff 0x000000   3
0x000000 0xff0000   4
0x000000 0x00ff00   5
0x000000 0x0000ff   6
```

**ES-4/ES-40**:  Five channels are split across 40 bits (S/PDIF
values arrive as 24-bit words in most systems, with the lower 4
bits always 0):
```
Bitmask (L R)       Channel 
0xff0000 0x000000   1
0x00ff00 0x000000   2
0x000000 0xff0000   3
0x000000 0x00ff00   4
0x0000f0 0x0000f0   5
```


### Expansion port to ESX-8GT 

The 8 bits of the expansion port signal are connected directly to
the 8 gate outputs.  Each word of digital output immediately affects the 
gate outputs, so they can change at audio rate (44.1 kHz/48 kHz)

```
Bit   1  2  3  4  5  6  7  8
Gate [1][2][3][4][5][6][7][8]
```

### Expansion port to ESX-8CV 

The 12 bits of each CV signal are encoded into 3 8-bit expansion
port words.  You may notice that this is using 24 bits to encode
12 bits of data, which seems a bit inefficient.  Also included is
the destination CV port number and some sync data to make the
connection robust and allow for the best possible sample rate for
the number of desired signals (from r/3 to r/24 depending on the
number of connected signals)

Word 1: 
```
Bit   Value  Meaning 
0x80     1   This is the first word
0x40     0   Not the second word
0x20     0   Or the third word
0x10    D5   
0x08    D4
0x04    D3
0x02    D2
0x01    D1 
```

Word 2: 
```
0x80     0   Not the first word
0x40     1   This is the second word
0x20     0   Not the third word
0x10   D10   
0x08    D9
0x04    D8
0x02    D7
0x01    D6 
```

Word 3: 
```
0x80     0   Not the first word
0x40     0   Not the second word
0x20     1   This is the third word
0x10    A2   CV address bit 2
0x08    A1   CV address bit 1
0x04    A0   CV address bit 0
0x02   D12
0x01   D11
```

I believe that the CV outputs arei each basically just a
sample-and-hold with a lowpass filter after it.  Each data point
addressed to a certain CV output is immediately available on that
output once the frame containing it is written. This allows for
the interesting property of the variable sample rate of the
output CVs, but it also means that this is probably not going to
be a very high-fidelity output for signals changing at more than
a few Hz. 

### Expansion port to ESX-4CV 

I'm not sure of the history, but I believe this was the first
iteration of representing a 12-bit value on an 8-bit channel.
The advertising material says that the sample rate of each port a
1/8th the audio rate, meaning that it takes 8 8-bit words to
represent 4 12-bit samples.  That means 16 bits (2 words) for
each 12-bit value, with 4 extras: 2 bits for sync and 2 bits for 
DAC address.  Thanks to Expert Sleepers for confirming this 
bit mapping in the support forum:

Word 1
```
Bit   Value  Meaning 
0x80     1   This is the first word
0x40    A1   CV address bit 1 
0x20    A0   CV address bit 0 
0x10   D12   (data >> 7)
0x08   D11
0x04   D10
0x02    D9
0x01    D8
```

Word 2
```
Bit   Value  Meaning 
0x80     0   This is the second word
0x40    D7   (data & 0x7f) 
0x20    D6   
0x10    D5   
0x08    D4
0x04    D3
0x02    D2
0x01    D1 
```

I would guess that, like the ESX-8CV, frames are requested but 
not required to cycle through all 4 CV addresses in a cycle. 
