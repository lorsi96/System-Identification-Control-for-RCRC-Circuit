#!python3
import numpy as np
import csv
import time
import matplotlib.pyplot as plt
import functools
from   matplotlib.animation import FuncAnimation
import io
from typing import Mapping, Any, Callable, BinaryIO, Dict, List, Tuple, Protocol
import serial

STREAM_FILE=("/dev/ttyUSB1", "serial")
OUTPUTFILEPATH='output.csv'
HeaderSpec = Mapping[str, Callable[[BinaryIO], int]]
Header = Mapping[str, Any]
SerialData = Tuple[Header, List[float], List[float]]

# ********************************* Parsers ********************************* #
def stdint_read(f:BinaryIO, size_bytes=2, signed=False) -> int:
    raw = bytes()
    for _ in range(size_bytes):
        raw += f.read(1)
    return int.from_bytes(raw, "little", signed=signed)

def hex_read(f:BinaryIO, size_bytes=2, signed=False):
    return hex(stdint_read(f, size_bytes, signed))


# ********************************* Streams ********************************* #
class Streamable(Protocol):
    def open(self) -> BinaryIO:
        ...
    
    def flush(self):
        ...

class SerialStreamable(Streamable):
    def __init__(self, port:str='/dev/ttyUSB1') -> None:
        self.port = port
    
    def open(self) -> BinaryIO:
        self.f = serial.Serial(port=self.port, baudrate=460800, timeout=None)
        return self.f  # type: ignore
    
    def flush(self):
        self.f.flushInput()

class FileStreamable(Streamable):
    def __init__(self, sz_bytes, filepath:str='serial') -> None:
        self.fpath = filepath
        self.sz = sz_bytes

    def open(self) -> BinaryIO:
        self.f = open(self.fpath, "rb", 0)
        self.flush()
        return self.f
    
    def flush(self):
        self.f.seek(2*self.sz, io.SEEK_END)

# ****************************** Serial Manager ***************************** #
class SerialHeaderManager:
    HEAD = b'head'
    TAIL = b'tail'
    def __init__(self, file:BinaryIO, spec: HeaderSpec, def_packet:Header) -> None:
        self._file = file 
        self._spec = spec 
        self._last_packet = def_packet
    
    def wait_for_packet(self) -> SerialData:
        return (
            self.__find_header(), 
            *self.__read_data(self._last_packet['N']))
    
    def __find_header(self) -> Mapping[str, Any]:
        found = False 
        ret: Dict[str, Any] = {"head": SerialHeaderManager.HEAD}
        while not found:
            self.__find_head()
            for key, parser in self._spec.items():
                ret.update({key: parser(self._file)})
            found = self.__find_tail()
        self._last_packet = ret
        return ret
    
    def __read_data(self, n:int) -> Tuple[List[float], List[float]] :
        parse = lambda: stdint_read(self._file, signed=True) / 0x7FFF
        ret = zip(*([parse(), parse()] for _ in range(n)))  # type: ignore
        return list(next(ret)), list(next(ret))

    def __find_head(self):
        data=bytearray(len(SerialHeaderManager.HEAD))
        while data!=SerialHeaderManager.HEAD:
            raw=self._file.read(1)
            data+=raw
            data[:]=data[-4:]

    def __find_tail(self):
        data=bytearray(b'1234')
        for _ in range(4):
            data+=self._file.read(1)
            data[:]=data[-4:]
        return data == SerialHeaderManager.TAIL

# ******************************** Data Dump ******************************** #


# ********************************* Plotting ******************************** #
# fig = plt.figure(1)
# adcAxe = fig.add_subplot ( 2,1,1                  )
# adcLn, = plt.plot        ( [],[],'r-',linewidth=4 )
# adcAxe.grid              ( True                   )
# adcAxe.set_ylim          ( -1.65 ,1.65            )
# adcAxe.set_title('Scope')
# adcAxe.set_xlabel('Time [Sec]')
# adcAxe.set_ylabel('Mag')

# dacAxe = fig.add_subplot ( 2,1,2                  )
# dacLn, = plt.plot        ( [],[],'b-',linewidth=4 )
# dacAxe.grid              ( True                   )
# # dacAxe.set_ylim          ( 0 ,0.25                )
# dacAxe.set_ylim          ( -1.65 ,1.65            )
# dacAxe.set_title('Scope')
# dacAxe.set_xlabel('Time [Sec]')
# dacAxe.set_ylabel('Mag')

# plt.tight_layout()

fig, ax = plt.subplots(figsize=(8, 6))


dacLn, = ax.plot([], [], 'b-', linewidth=2, label='DAC Data')
adcLn, = ax.plot([], [], 'r-', linewidth=2, label='ADC Data')

ax.grid(True)
ax.set_ylim(-1.1, 1.1)
ax.set_title('Scope')
ax.set_xlabel('Time [Sec]')
ax.set_ylabel('Mag')
ax.legend(loc='upper right')

plt.tight_layout()

COEFFSN = 10 
header = {
    "head": b"head", 
    "id": 0, 
    "N":512, 
    "fs": 1000,
    # **{f"coeff{n}": 0 for n in range(COEFFSN)}, 
    # "dbg1": 0,
    # "dbg2": 0,
    # "dbg3": 0,
    "tail":b"tail"
}

header_spec = {
    "id": functools.partial(stdint_read, size_bytes=4), 
    "N": stdint_read, 
    "fs": stdint_read, 
    # **{f"coeff{n}": hex_read for n in range(COEFFSN)}, 
    # "dbg1": hex_read,
    # "dbg2": hex_read,
    "dbg3": hex_read,
}


outfile = open(OUTPUTFILEPATH, 'w')
csv_writer = csv.DictWriter(outfile, fieldnames=('Time', 'ADC', 'DAC',))
csv_writer.writeheader()
stream = SerialStreamable()
stream_file = stream.open()
serial_manager = SerialHeaderManager(stream_file, header_spec, header)

rec=np.ndarray(1).astype(np.int16)
timestamp=0

def init():
    global rec
    rec=np.ndarray(1).astype(np.int16)
    return adcLn, dacLn

def update(t):
    global header,rec, outfile, timestamp
    found_h, raw_data_b, raw_data  = serial_manager.wait_for_packet() 
    raw_data_b = raw_data_b[1:] + [raw_data_b[-1]]
    print(found_h)
    id, N, fs = found_h["id"], found_h["N"], found_h["fs"]
    
    adc   = np.array(raw_data)
    dac   =  np.array(raw_data_b)
    time  = np.arange(0, N/fs, 1/fs)
    time_vect =  time + timestamp
    timestamp += N/fs
    csv_writer.writerows(map(lambda v: {'Time': v[2], 'ADC': v[0], 'DAC': v[1]}, zip(adc, dac, time_vect)))

    ax.set_xlim (0, N/fs)
    dacLn.set_data(time, dac)
    adcLn.set_data(time, adc)

    rec=np.concatenate((rec,((adc/1.65)*2**(15-1)).astype(np.int16)))

    return adcLn, dacLn


if __name__ == '__main__':
    ani=FuncAnimation(fig, update, 128, init_func=init, blit=True, interval=1, 
                    repeat=True)
    plt.draw()
    plt.show()
    stream_file.close()
    outfile.close()
