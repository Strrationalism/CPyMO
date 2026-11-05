"""MIT. Build the optional Windows host encoder; players need no compiler."""
from pathlib import Path
import argparse,subprocess,hashlib,json
root=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--cc',type=Path,required=True);a=parser.parse_args()
command=[str(a.cc.resolve()),'-shared','-static','-O3',str(root/'lossless-native.c'),'-o',str(root/'lossless-native.dll')]
subprocess.run(command,check=True)
print(json.dumps({'command':command,'sha256':hashlib.sha256((root/'lossless-native.dll').read_bytes()).hexdigest()}))
