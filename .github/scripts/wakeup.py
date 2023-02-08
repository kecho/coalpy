import wakeonlan as w
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python -m coalpyci.wakeup",
        description="Wakes up a server sending a magical packet.")
    parser.add_argument('macaddress')
    parser.add_argument('-s', '--server',
        default='localhost', required=False)
    parser.add_argument('-p', '--port',
        default='9', required=False)
    args = parser.parse_args()

    w.send_magic_packet(args.macaddress, ip_address=args.server, port=int(args.port))
