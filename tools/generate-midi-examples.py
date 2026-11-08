#!/usr/bin/env python3
"""Generate three original, royalty-free Cardputer MIDI acceptance sequences.
Requires Python 3 only. No external MIDI library or SoundFont required.
Usage: python3 tools/generate-midi-examples.py [destination-directory]
"""
from pathlib import Path
import struct
import sys

outdir=Path(sys.argv[1] if len(sys.argv)>1 else "midi-test-songs")
outdir.mkdir(parents=True, exist_ok=True)

def vlq(n):
    if n<0 or n>0x0fffffff: raise ValueError("invalid delta")
    data=[n&0x7f]
    n>>=7
    while n:
        data.insert(0,(n&0x7f)|0x80)
        n>>=7
    return bytes(data)

def miditrack(name,events):
    events=[(0,0,bytes([0xff,0x03])+vlq(len(name))+name.encode("ascii"))]+events
    events.sort(key=lambda v:(v[0],v[1]))
    body=bytearray()
    prev=0
    for tick,order,msg in events:
        body+=vlq(tick-prev)+msg
        prev=tick
    body+=bytes([0,0xff,0x2f,0])
    return b"MTrk"+struct.pack(">I",len(body))+body

def file(name,fmt,tracks,ppqn=480):
    header=b"MThd"+struct.pack(">IHHH",6,fmt,len(tracks),ppqn)
    data=header+b"".join(tracks)
    path=outdir/name
    path.write_bytes(data)
    print(path,len(data),"bytes")

def tempo(bpm):
    return bytes([0xff,0x51,0x03])+int(60000000/bpm).to_bytes(3,"big")

meta=[(0,1,bytes([0xff,0x58,4,4,2,24,8])),
      (0,2,tempo(122)),(4*1920,2,tempo(100)),(8*1920,2,tempo(135))]
lead=[(0,1,bytes([0xc0,81]))]
chords=[(0,1,bytes([0xc1,11]))]
bass=[(0,1,bytes([0xc2,33]))]
drums=[]
phrase=[72,76,79,76,74,77,81,79,76,74,72,67,69,72,76,74]
roots=[48,45,53,43,48,50,53,55,48,45,53,43]
for bar in range(12):
    base=bar*1920
    root=roots[bar]
    for p in (root+12,root+16,root+19):
        chords.extend([(base+8,3,bytes([0x91,p,58])),
                       (base+1700,1,bytes([0x81,p,0]))])
    for beat in range(4):
        tick=base+beat*480
        pitch=phrase[(bar*4+beat)%len(phrase)]
        lead.extend([(tick,3,bytes([0x90,pitch,94])),
                     (tick+355,1,bytes([0x80,pitch,0]))])
        bpitch=root if beat%2==0 else root+7
        bass.extend([(tick,3,bytes([0x92,bpitch,90])),
                     (tick+410,1,bytes([0x82,bpitch,0]))])
        drums.extend([(tick,3,bytes([0x99,36,96])),
                      (tick+90,1,bytes([0x89,36,0])),
                      (tick+240,3,bytes([0x99,42,47])),
                      (tick+300,1,bytes([0x89,42,0]))])
        if beat in (1,3):
            drums.extend([(tick,3,bytes([0x99,38,100])),
                          (tick+100,1,bytes([0x89,38,0]))])
file("01-Cardputer-Original-Theme.mid",1,[
    miditrack("Conductor",meta),miditrack("Lead",lead),
    miditrack("Chords",chords),miditrack("Bass",bass),
    miditrack("Drums",drums)
])

stress=[(0,0,bytes([0xc0,0])),(0,1,bytes([0xb0,64,127]))]
for note in range(48,60):
    stress.append((0,3,bytes([0x90,note,90])))
stress.append((960,1,bytes([0xe0,0,96])))
for note in range(48,60):
    stress.append((1440,1,bytes([0x80,note,0])))
stress.append((1920,1,bytes([0xb0,64,0])))
file("02-Cardputer-12-Voice-Stress.mid",0,[miditrack("Polyphony",stress)])

simple=[]
for i,n in enumerate((60,62,64,67,69,67,64,60)):
    simple.extend([(i*240,3,bytes([0x90,n,96])),
                   (i*240+185,1,bytes([0x80,n,0]))])
file("03-Cardputer-SMF0-Basic.mid",0,[miditrack("SMF0",simple)])
