import argparse
import numpy as np
from utils import cformatnbyn, zigzag, zigzaginv


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Print a zigzag array"
    )
    parser.add_argument("n", nargs='?', type=int, default=8)
    parser.add_argument("--inverse", action='store_true', default=False)
    args = parser.parse_args()
    if args.inverse:
      print(cformatnbyn(zigzaginv(args.n), name="zigzag_inv", n=args.n))
    else:
      print(cformatnbyn(zigzag(args.n), name="zigzag", n=args.n))