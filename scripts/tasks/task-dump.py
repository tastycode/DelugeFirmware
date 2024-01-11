#! /usr/bin/env python3
from datetime import datetime
from iterfzf import iterfzf
from pathlib import Path
import argparse
import mido
import os
import subprocess
import sys

DEBUG_SYSEX_ID = 0x7D
DELUGE_PORT_DEFAULT_IN = "Deluge IN"
DELUGE_PORT_DEFAULT_OUT = "Deluge OUT"

# b.mtime
# gcc -shared -o scripts/util/pack.so -fPIC src/deluge/util/pack.c

def argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="sysex-console",
        description="Listens for deluge sysex and prints console messages"
    )
    parser.group = "Development"
    parser.add_argument(
        "-I", "--input", help="The device we are reading from", default=DELUGE_PORT_DEFAULT_IN
    )
    parser.add_argument(
        "-O", "--output", help="The device we notify we'll be reading from so they know to read to us", default=DELUGE_PORT_DEFAULT_OUT
    )

    return parser

def is_verbose_mode() -> bool:
    return os.environ.get('SYSEX_CONSOLE_VERBOSE', False) or os.environ.get('DBT_VERBOSE', False)

def run_command(command: list, verbose: bool) -> None:
    if verbose:
        print(f"Executing command: {' '.join(command)}")

    result = subprocess.run(command, capture_output=True, text=True)
    
    if verbose:
        print(f"Exit code: {result.returncode}")
        print(f"Output: {result.stdout}")
        if result.returncode != 0:
            print(f"Error: {result.stderr}")

    result.check_returncode()

def unpack_7bit_to_8bit(bytes):
  output = bytearray()

  for b in bytes:
    # Extract 7-bit value
    value = b & 0x7F
    
    # Extend to 8-bit
    if value & 0x40:
      value |= 0x80
    
    output.append(value)

    result = []
    for byte in bytes:
        result.append(byte & 0x7f)
        result.append(byte >> 7)
    return bytearray(result)
def applyMask(buffer, c, o):
    mask = 0x7F  # 7F in hexadecimal is 0111 1111 in binary
    new_buffer = 0

    # Applying the mask and shifting bits according to c and o
    for i in range(8):  # Assuming a maximum of 8 segments as in an unsigned long long
        part = (buffer >> (c * i)) & mask
        new_buffer |= part << (o * i)

    return new_buffer

import struct

def encode_buffered(input_bytes, size, from_size, to_size):
    c = from_size
    o = to_size
    output_bytes = bytearray()

    i = size
    memsize = c if i > c else i

    while i > 0:
        # Extract the relevant bytes
        buffer_bytes = input_bytes[:memsize]
        input_bytes = input_bytes[memsize:]

        # Convert bytes to an unsigned long long
        buffer = struct.unpack('<Q', buffer_bytes + b'\x00' * (8 - len(buffer_bytes)))[0]

        # Apply the mask
        applyMask(buffer, c, o)

        # Convert back to bytes and append to output
        output_bytes.extend(struct.pack('<Q', buffer)[:o])

        # Update loop variables
        i -= memsize
        memsize = c if i > c else i

    return output_bytes

def encoded_size(inputSize, fromSize, toSize):
    return math.ceil(inputSize * fromSize / toSize)


def main() -> int:
    args = argparser().parse_args()
    verbose = is_verbose_mode()

    input_ports=mido.get_input_names()
    print(f"Available input_ports: {input_ports}")
    input_device = args.input
    if input_device not in input_ports:
        def iter_input_ports():
            for port in input_ports:
                yield port
        input_device = iterfzf(iter_input_ports(), prompt = "'{device}' not found, please choose a port")

    output_ports=mido.get_output_names()
    print(f"Available output_ports: {output_ports}")
    output_device = args.output
    if output_device not in output_ports:
        def iter_output_ports():
            for port in output_ports:
                yield port
        output_device = iterfzf(iter_output_ports(), prompt = "'{device}' not found, please choose a port")

    now = datetime.now()  
    timestamp = now.isoformat()
    if input_device is None or output_device is None:
        print("No device selected")
        return 1
    print(f"Using device: {input_device}/{output_device}")
    with mido.open_input(input_device) as port:
        target_length = 0
        with mido.open_output(output_device) as output:
            output.send(mido.Message.from_bytes([
                0xf0,  # sysex message
                0x7d, # deluge (midi_engine.cpp midiSysexReceived)
                0x03,  # debug namespace
                0x03,   # sysex.cpp, sysexEreceived
                0x00,
                0xf7]))
            chunks = []
            print("open port")
            for message in port:
                packet_length = len(message.bytes())
                chunks_length = sum([len(chunk) for chunk in chunks])
                if chunks_length > 0 and chunks_length >= target_length:
                    break
                header_size = 7
                is_dump_message = False
                try:
                    is_dump_message = message.type == 'sysex'  and message.bytes()[0:4] == [0xf0, 0x7d, 0x03, 0x30] #[0xf0, 0x7d, 0x03, 0x30] 
                except:
                    print(message)
                    print(message.hex())
                    print(f"packet_length: %d", packet_length)
                #print(f"--{message.bytes()[0:4]}-- {message.bytes()[0] == 0xf0} ")
                #print(f"target_length: {target_length}, chunks_len: {len(chunks)}, is_dump_message: {is_dump_message}")
                if is_dump_message:
                    print("is_dump_message!")
                    with open('./dump.syx', 'wb') as file:
                        file.write(message.bin())
                    if len(chunks)==0:
                        first_place = message.bytes()[4]
                        second_place = message.bytes()[5]
                        target_length = first_place * 256 + second_place
                        print(f"target_length: {target_length}, chunks_len: {len(chunks)}")
                    print("128")

                    target_bytes = encode_buffered(message.bin()[header_size:-1], target_length, 7, 8)    
                    # unpack_7bit_to_8bit(message.bin()[header_size-1:-1])
                    print("130")
                    print(f"target_bytes: len {len(target_bytes)}")

                    if target_bytes is not None:
                        decoded = target_bytes.decode('ascii', errors = 'ignore')
                        print("132")
                        print(decoded)
                        print("136")
                        chunks.append(decoded)
                    else:
                        print("None")
                
                print(" 🥹 ".join(chunks))
      #sys.stdout.write(decoded.replace("\n", f"\n[{timestamp}] "))

    return 0

if __name__ == "__main__":
    main()
